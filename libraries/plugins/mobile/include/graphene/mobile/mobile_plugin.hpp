#pragma once

#include <graphene/app/plugin.hpp>

/*
 * Our basic namespace
 */
namespace graphene { namespace mobile_plugin {

namespace detail {
 /* The "detail" is used to namespace the actual implemention and separated it
  * from the interface
  */
 class mobile_plugin_impl;
}

/*
 * We inherit all the nice features of graphene::app::plugin
 */
class mobile_plugin : public graphene::app::plugin
{
   public:
      /**
       * The plugin requires a constructor which takes app. This is called regardless of whether the plugin is loaded.
       * The app parameter should be passed up to the superclass constructor.
       */
      mobile_plugin();

      /**
       * Plugin is destroyed via base class pointer, so a virtual destructor must be provided.
       */
      ~mobile_plugin();

      /**
       * Every plugin needs a name.
       */
      std::string plugin_name()const override;

      /**
       * Called when the plugin is enabled, but before the database has been created.
       */
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;

      /**
       * Called when the plugin is enabled.
       */
      virtual void plugin_startup() override;

      /**
       * Called when the plugin is shut down.
       */
      virtual void plugin_shutdown() override;

   private:
      /*
       * The implementation instance is stored privately in `my`
       */
      std::shared_ptr< detail::mobile_plugin_impl > my;
};

} }
