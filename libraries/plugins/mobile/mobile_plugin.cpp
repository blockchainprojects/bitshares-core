#include <graphene/mobile/mobile_api.hpp>
#include <graphene/mobile/mobile_plugin.hpp>

#include <string>

namespace graphene { namespace mobile_plugin {

/*
 * The detail is used to separated the implementation from the interface
 */
namespace detail {

class mobile_plugin_impl
{
   public:
      mobile_plugin_impl();
      virtual ~mobile_plugin_impl();

      virtual std::string plugin_name()const;
      virtual void plugin_initialize( const boost::program_options::variables_map& options );
      virtual void plugin_startup();
      virtual void plugin_shutdown();

      // TODO:  Add custom methods here
};

/**
 * Constructor Implementation
 */
mobile_plugin_impl::mobile_plugin_impl()
{
}

/**
 * Destructor Implementation
 */
mobile_plugin_impl::~mobile_plugin_impl() {}

/**
 * Get plugin name Implementation
 */
std::string mobile_plugin_impl::plugin_name()const
{
   return "mobile_api";
}

/*
 * Initialize Implementation
 */
void mobile_plugin_impl::plugin_initialize( const boost::program_options::variables_map& options )
{
   ilog("mobile plugin:  plugin_initialize()");
}

/*
 * Plugin Startup implementation
 */
void mobile_plugin_impl::plugin_startup()
{
   ilog("mobile plugin:  plugin_startup()");
}

/*
 * Plugin Shutdown implementation
 */
void mobile_plugin_impl::plugin_shutdown()
{
   ilog("mobile plugin:  plugin_shutdown()");
}

} /// detail

/*
 * Now we are done with implementation/detail, let's do the interface (linking
 * implementation)
 */


mobile_plugin::mobile_plugin() :
   my( new detail::mobile_plugin_impl )
{
}

mobile_plugin::~mobile_plugin() {}

std::string mobile_plugin::plugin_name()const
{
   return my->plugin_name();
}

void mobile_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   my->plugin_initialize( options );
}

void mobile_plugin::plugin_startup()
{
   my->plugin_startup();
}

void mobile_plugin::plugin_shutdown()
{
   my->plugin_shutdown();
}

} } // graphene::mobile_plugin
