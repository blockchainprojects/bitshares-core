/*
 * Copyright (c) 2018 net_stats and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/net_stats/net_stats.hpp>
#include <graphene/net/node.hpp>

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>

#include <boost/bind.hpp>

namespace graphene { namespace net_stats {

namespace detail {

class net_stats_impl {
public:
    net_stats_impl(net_stats_plugin& _plugin)
        : _self(_plugin) 
        {
            // TODO ipaddr and port not static?
            exposer  = std::make_shared<prometheus::Exposer>( "127.0.0.1:8080" );
            registry = std::make_shared<prometheus::Registry>();


            auto& msg_cnt_fam = prometheus::BuildCounter()
                                    .Name( "p2p_message_cnt" )
                                    .Help( "How much messages has this node received/transmitted?" )
                                    .Labels( {} )
                                    .Register( *registry );

            msg_rx_cnt = &msg_cnt_fam.Add( { {"type", "rx"} } );
            msg_tx_cnt = &msg_cnt_fam.Add( { {"type", "tx"} } );



            auto& msg_size_hsg_fam = prometheus::BuildHistogram()
                                        .Name( "p2p_message_size_hsg" )
                                        .Help( "How much messages with which size were received/transmitted?" )
                                        .Labels( {} )
                                        .Register( *registry );
            
            prometheus::Histogram::BucketBoundaries msg_size_boundaries;
            for( uint16_t bucket = 256; bucket <= 4096; bucket += 256 )
                msg_size_boundaries.push_back( bucket );

            msg_rx_size_hsg = &msg_size_hsg_fam.Add( { {"type", "rx"} }, msg_size_boundaries );
            msg_tx_size_hsg = &msg_size_hsg_fam.Add( { {"type", "tx"} }, msg_size_boundaries );



            auto& block_latency_hsg_fam = prometheus::BuildHistogram()
                                                        .Name( "p2p_block_latency_hsg" )
                                                        .Help( "How much block messages with which latency were received?" )
                                                        .Labels( {} )
                                                        .Register( *registry );
            
            prometheus::Histogram::BucketBoundaries block_latency_boundaries;
            // TODO choose better buckets testnet buckts are calculated with .now() == ~2016
            for( double bucket = 71000000000.0; bucket < 82000000000.0; bucket += 500000000.0 )
                block_latency_boundaries.push_back( bucket );

            block_latency_hsg = &block_latency_hsg_fam.Add( {}, block_latency_boundaries );
            


            exposer->RegisterCollectable( registry );
        }
    ~net_stats_impl() = default;

    net_stats_plugin& _self;

    void process_event(const net::network_statistics_event& event) {
        //ilog("Network statistic event: type=${type}, size=${size}, peer=${peer}", ("type", event.event_type)("size", event.event_data.size())("peer", event.remote_endpoint));
        switch( event.event_type )
        {
            case net::network_statistics_event::EventType::MessageReceived:
            {
                msg_rx_cnt->Increment();  

                msg_rx_size_hsg->Observe( event.event_data.size() );

                break;
            }
            case net::network_statistics_event::EventType::MessageSent:
            {
                msg_tx_cnt->Increment();

                msg_tx_size_hsg->Observe( event.event_data.size() );

                break;
            }
            default:
                break;

        }
    }

    void on_block_handled( int64_t block_latency ) 
    {
        block_latency_hsg->Observe( block_latency );
    }

private:
    // http server to expose prometheus metrics
    std::shared_ptr<prometheus::Exposer> exposer;
    // registry for all metric types
    std::shared_ptr<prometheus::Registry> registry;
    // p2p received message counter
    prometheus::Counter* msg_rx_cnt;
    // p2p transmitted message counter
    prometheus::Counter* msg_tx_cnt;
    // p2p received message size histogram 
    prometheus::Histogram* msg_rx_size_hsg;
    // p2p transmitted message size histogram
    prometheus::Histogram* msg_tx_size_hsg;
    // p2p block latency
    prometheus::Histogram* block_latency_hsg;
};
} // end namespace detail

net_stats_plugin::net_stats_plugin() :
    my(new detail::net_stats_impl(*this)) {
}

net_stats_plugin::~net_stats_plugin() = default;

std::string net_stats_plugin::plugin_name()const {
    return "net_stats";
}

std::string net_stats_plugin::plugin_description()const {
    return "Plugin to receive and process statistics events from P2P network";
}

void net_stats_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                  boost::program_options::options_description& cfg) {
//    cli.add_options()
//            ("net_stats_option", boost::program_options::value<std::string>(), "net_stats option")
//            ;
//    cfg.add(cli);
}

void net_stats_plugin::plugin_startup() {
    ilog("net_stats: plugin_startup() begin");
    FC_ASSERT(&p2p_node() != nullptr, "P2P node not yet set! Unable to initialize");
    p2p_node().subscribe_network_stats([this](const net::network_statistics_event& event) {
        // Copy event and schedule processing for later; return immediately to unblock network
        fc::async([event, data=event.event_data, this]() {
            net::network_statistics_event copy(event.event_type, event.remote_endpoint, data, event.event_time);
            my->process_event(copy);
        });
    });
    // register block_handled signal to emit the block_latency
    // TODO save return for disconnect
    app().block_handled.connect( boost::bind( &detail::net_stats_impl::on_block_handled, &*my, _1 ) );
}

} }
