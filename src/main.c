/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
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

#include <ibus.h>
#include "engine.h"

static gboolean ibus = FALSE;
static gboolean verbose = FALSE;

static const GOptionEntry entries[] =
{
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose", NULL },
    { NULL }
};

static void disconnect(IBusBus *bus, gpointer data)
{
    ibus_quit();
}

int main(int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    
    IBusBus *bus;
    IBusFactory *factory;
    IBusComponent *component;
    
    context = g_option_context_new("- ibus rtk engine");
    g_option_context_add_main_entries(context, entries, "ibus-rtk");
    
    if(!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print("Option parsing failed: %s\n", error->message);
        g_error_free(error);
        return -1;
    }
    
    ibus_init();
    
    bus = ibus_bus_new();
    g_object_ref_sink(bus);
    g_signal_connect(bus, "disconnected", G_CALLBACK(disconnect), NULL);
    
    factory = ibus_factory_new(ibus_bus_get_connection(bus));
    g_object_ref_sink(factory);
    ibus_factory_add_engine(factory, "rtk", IBUS_TYPE_RTK_ENGINE);
    
    if(ibus)
        ibus_bus_request_name(bus, "org.freedesktop.IBus.RTK", 0);
    else
    {
        component = ibus_component_new_from_file("rtk.xml");
        ibus_bus_register_component(bus, component);
        g_object_unref(component);
    }
    
    ibus_main();
    
    return 0;
}
