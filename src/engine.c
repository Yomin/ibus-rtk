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

#include "engine.h"

typedef struct _IBusRTKEngine IBusRTKEngine;
typedef struct _IBusRTKEngineClass IBusRTKEngineClass;

struct _IBusRTKEngine
{
    IBusEngine parent;
};

struct _IBusRTKEngineClass
{
    IBusEngineClass parent;
};


static void ibus_rtk_engine_class_init(IBusRTKEngineClass *klass);
static void ibus_rtk_engine_init(IBusRTKEngine *rtk);
static void ibus_rtk_engine_destroy(IBusRTKEngine *rtk);
static gboolean ibus_rtk_engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint modifiers);


G_DEFINE_TYPE(IBusRTKEngine, ibus_rtk_engine, IBUS_TYPE_ENGINE)

static void ibus_rtk_engine_class_init(IBusRTKEngineClass *klass)
{
    IBUS_OBJECT_CLASS(klass)->destroy = (IBusObjectDestroyFunc)ibus_rtk_engine_destroy;
    IBUS_ENGINE_CLASS(klass)->process_key_event = ibus_rtk_engine_process_key_event;
}

static void ibus_rtk_engine_init(IBusRTKEngine *rtk)
{
    
}

static void ibus_rtk_engine_destroy(IBusRTKEngine *rtk)
{
    ((IBusObjectClass*)ibus_rtk_engine_parent_class)->destroy((IBusObject*)rtk);
}

static gboolean ibus_rtk_engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint modifiers)
{
    return FALSE;
}
