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
#include "lookup.h"

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))

extern gchar *dict;

typedef struct _IBusRTKEngine IBusRTKEngine;
typedef struct _IBusRTKEngineClass IBusRTKEngineClass;

struct _IBusRTKEngine
{
    IBusEngine parent;
    IBusLookupTable *table;
    GString *preedit, *prekanji;
    gint cursor;
    struct rtkresult *lookup;
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
    rtk->preedit = g_string_new("");
    rtk->prekanji = g_string_new("");
    rtk->cursor = 0;
    rtk->table = ibus_lookup_table_new(10, 0, TRUE, TRUE);
    g_object_ref_sink(rtk->table);
    
    rtk_lookup_init(dict);
}

static void ibus_rtk_engine_destroy(IBusRTKEngine *rtk)
{
    if(rtk->preedit)
        g_string_free(rtk->preedit, TRUE);
    if(rtk->prekanji)
        g_string_free(rtk->prekanji, TRUE);
    if(rtk->table)
        g_object_unref(rtk->table);
    rtk_lookup_free();
    ((IBusObjectClass*)ibus_rtk_engine_parent_class)->destroy((IBusObject*)rtk);
}

static void ibus_rtk_engine_reset(IBusRTKEngine *rtk)
{
    g_string_assign(rtk->preedit, "");
    g_string_assign(rtk->prekanji, "");
    rtk->cursor = 0;
    
    ibus_engine_hide_preedit_text((IBusEngine*)rtk);
    ibus_engine_hide_auxiliary_text((IBusEngine*)rtk);
    ibus_engine_hide_lookup_table((IBusEngine*)rtk);
}

static void ibus_rtk_engine_update_preedit(IBusRTKEngine *rtk, gboolean red)
{
    IBusText *text;
    
    text = ibus_text_new_from_static_string(rtk->preedit->str);
    text->attrs = ibus_attr_list_new();
    ibus_attr_list_append(text->attrs,
        ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, 0, rtk->preedit->len));
    if(red)
        ibus_attr_list_append(text->attrs,
            ibus_attr_foreground_new(0xff0000, 0, rtk->cursor));
    
    ibus_engine_update_preedit_text((IBusEngine*)rtk, text, rtk->cursor, TRUE);
    
    g_string_assign(rtk->prekanji, "");
    ibus_engine_hide_auxiliary_text((IBusEngine*)rtk);
    ibus_engine_hide_lookup_table((IBusEngine*)rtk);
}

static void ibus_rtk_engine_update_prekanji(IBusRTKEngine *rtk)
{
    IBusText *text;
    
    text = ibus_text_new_from_static_string(rtk->prekanji->str);
    
    ibus_engine_update_preedit_text((IBusEngine*)rtk, text, rtk->prekanji->len, TRUE);
    ibus_engine_show_lookup_table((IBusEngine*)rtk);
}

static gboolean ibus_rtk_engine_commit(IBusRTKEngine *rtk, GString *str)
{
    IBusText *text;
    
    if(!str->len)
        return FALSE;
    
    text = ibus_text_new_from_static_string(str->str);
    ibus_engine_commit_text((IBusEngine*)rtk, text);
    
    ibus_rtk_engine_reset(rtk);
    
    return TRUE;
}

static void ibus_rtk_engine_update_lookup(IBusRTKEngine *rtk)
{
    IBusText *text;
    guint pos;
    
    pos = ibus_lookup_table_get_cursor_pos(rtk->table);
    text = ibus_text_new_from_printf("%i / %i", pos+1,
        ibus_lookup_table_get_number_of_candidates(rtk->table));
    
    g_string_assign(rtk->prekanji, rtk->lookup[pos].kanji);
    ibus_rtk_engine_update_prekanji(rtk);
    
    ibus_engine_update_auxiliary_text((IBusEngine*)rtk, text, TRUE);
    ibus_engine_update_lookup_table((IBusEngine*)rtk, rtk->table, TRUE);
}

static gboolean ibus_rtk_engine_lookup(IBusRTKEngine *rtk)
{
    IBusText *text;
    struct rtkresult *result;
    
    if(!(rtk->lookup = result = rtk_lookup(1, &rtk->preedit->str)))
        return FALSE;
    
    ibus_lookup_table_clear(rtk->table);
    
    while(result->kanji)
    {
        text = ibus_text_new_from_printf("%s: %s", result->kanji, result->meaning);
        ibus_lookup_table_append_candidate(rtk->table, text);
        result++;
    }
    
    ibus_rtk_engine_update_lookup(rtk);
    
    return TRUE;
}

static gboolean ibus_rtk_engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint modifiers)
{
    IBusRTKEngine *rtk = (IBusRTKEngine*)engine;
    
    if(modifiers)
        return rtk->cursor ? TRUE : FALSE;
    
    switch(keyval)
    {
    case IBUS_space:
        if(rtk->prekanji->len)
        {
            ibus_lookup_table_cursor_down(rtk->table);
            ibus_rtk_engine_update_lookup(rtk);
            return TRUE;
        }
        if(rtk->preedit->len)
        {
            if(!ibus_rtk_engine_lookup(rtk))
                ibus_rtk_engine_update_preedit(rtk, TRUE);
            return TRUE;
        }
        return FALSE;
    case IBUS_Return:
        if(rtk->prekanji->len)
            return ibus_rtk_engine_commit(rtk, rtk->prekanji);
        else
            return ibus_rtk_engine_commit(rtk, rtk->preedit);
    case IBUS_Escape:
        if(!rtk->preedit->len)
            return FALSE;
        ibus_rtk_engine_reset(rtk);
        return TRUE;
    case IBUS_BackSpace:
        if(!rtk->preedit->len)
            return FALSE;
        if(rtk->cursor > 0)
        {
            rtk->cursor--;
            g_string_erase(rtk->preedit, rtk->cursor, 1);
            ibus_rtk_engine_update_preedit(rtk, FALSE);
        }
        return TRUE;
    case IBUS_Delete:
        if(!rtk->preedit->len)
            return FALSE;
        if(rtk->cursor < rtk->preedit->len)
        {
            g_string_erase(rtk->preedit, rtk->cursor, 1);
            ibus_rtk_engine_update_preedit(rtk, FALSE);
        }
        return TRUE;
    case IBUS_Down:
        if(!rtk->prekanji->len)
            return FALSE;
        ibus_lookup_table_cursor_down(rtk->table);
        ibus_rtk_engine_update_lookup(rtk);
        return TRUE;
    case IBUS_Up:
        if(!rtk->prekanji->len)
            return FALSE;
        ibus_lookup_table_cursor_up(rtk->table);
        ibus_rtk_engine_update_lookup(rtk);
        return TRUE;
    default:
        if(is_alpha(keyval))
        {
            g_string_insert_c(rtk->preedit, rtk->cursor, keyval);
            rtk->cursor++;
            ibus_rtk_engine_update_preedit(rtk, FALSE);
            return TRUE;
        }
    }
    
    return FALSE;
}
