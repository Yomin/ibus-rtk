/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
 *
 * This file is derived from ibus-tmpl/src/engine.c
 * by Peng Huang <shawn.p.huang@gmail.com>
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
#define primitive_current(n) (g_array_index(rtk->primitives, GString*, rtk->primitive_current+(n)))

extern gchar *dict;

typedef struct _IBusRTKEngine IBusRTKEngine;
typedef struct _IBusRTKEngineClass IBusRTKEngineClass;

struct _IBusRTKEngine
{
    IBusEngine parent;
    IBusLookupTable *table;
    GString *preedit, *prekanji;
    gint cursor, primitive_count, primitive_current, primitive_cursor;
    struct rtkresult *lookup;
    GArray *primitives;
};

struct _IBusRTKEngineClass
{
    IBusEngineClass parent;
};


static void ibus_rtk_engine_class_init(IBusRTKEngineClass *klass);
static void ibus_rtk_engine_init(IBusRTKEngine *rtk);
static void ibus_rtk_engine_destroy(IBusRTKEngine *rtk);
static void ibus_rtk_engine_focus_in(IBusEngine *engine);
static gboolean ibus_rtk_engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint modifiers);


G_DEFINE_TYPE(IBusRTKEngine, ibus_rtk_engine, IBUS_TYPE_ENGINE)

static void ibus_rtk_engine_class_init(IBusRTKEngineClass *klass)
{
    IBUS_OBJECT_CLASS(klass)->destroy = (IBusObjectDestroyFunc)ibus_rtk_engine_destroy;
    IBUS_ENGINE_CLASS(klass)->process_key_event = ibus_rtk_engine_process_key_event;
    IBUS_ENGINE_CLASS(klass)->focus_in = ibus_rtk_engine_focus_in;
}

static void ibus_rtk_engine_primitive_free(gpointer data)
{
    g_string_free(*(GString**)data, TRUE);
}

static void ibus_rtk_engine_init(IBusRTKEngine *rtk)
{
    GString *str;
    IBusText *text;
    const char *labels[] = {"一", "二", "三", "四", "五", "六", "七", "八", "九", "十", 0};
    char **label = (char**)labels;
    
    rtk->preedit = g_string_new("");
    rtk->prekanji = g_string_new("");
    rtk->cursor = 0;
    
    rtk->table = ibus_lookup_table_new(10, 0, TRUE, TRUE);
    g_object_ref_sink(rtk->table);
    
    while(*label)
    {
        text = ibus_text_new_from_printf("%s", *(label++));
        ibus_lookup_table_append_label(rtk->table, text);
    }
    
    rtk->primitives = g_array_new(FALSE, FALSE, sizeof(GString*));
    str = g_string_new("");
    g_array_append_val(rtk->primitives, str);
    rtk->primitive_count = 1;
    rtk->primitive_current = 0;
    rtk->primitive_cursor = 0;
    g_array_set_clear_func(rtk->primitives, ibus_rtk_engine_primitive_free);
    
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
    if(rtk->primitives)
        g_array_free(rtk->primitives, TRUE);
    rtk_lookup_free();
    ((IBusObjectClass*)ibus_rtk_engine_parent_class)->destroy((IBusObject*)rtk);
}

static void ibus_rtk_engine_reset(IBusRTKEngine *rtk)
{
    guint x;
    
    g_string_assign(rtk->preedit, "");
    g_string_assign(rtk->prekanji, "");
    rtk->cursor = 0;
    
    if(rtk->primitive_count > 1)
        g_array_remove_range(rtk->primitives, 1, rtk->primitive_count-1);
    rtk->primitive_count = 1;
    rtk->primitive_current = 0;
    rtk->primitive_cursor = 0;
    g_string_assign(primitive_current(0), "");
    
    ibus_engine_hide_preedit_text((IBusEngine*)rtk);
    ibus_engine_hide_auxiliary_text((IBusEngine*)rtk);
    ibus_engine_hide_lookup_table((IBusEngine*)rtk);
}

static void ibus_rtk_engine_update_preedit(IBusRTKEngine *rtk, struct rtkinput *input)
{
    IBusText *text;
    guint x, pos, len;
    
    text = ibus_text_new_from_static_string(rtk->preedit->str);
    text->attrs = ibus_attr_list_new();
    
    for(x=0, pos=0; x<rtk->primitive_count; x++)
    {
        len = g_array_index(rtk->primitives, GString*, x)->len;
        ibus_attr_list_append(text->attrs,
            ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, pos, pos+len));
        if(input)
        {
            if(!input[x].found)
                ibus_attr_list_append(text->attrs,
                    ibus_attr_foreground_new(0xff0000, pos, pos+len));
            else
                ibus_attr_list_append(text->attrs,
                    ibus_attr_foreground_new(0x00ff00, pos, pos+len));
        }
        pos += len+1;
    }
    
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

static void ibus_rtk_engine_commit(IBusRTKEngine *rtk, GString *str)
{
    IBusText *text;
    
    text = ibus_text_new_from_static_string(str->str);
    ibus_engine_commit_text((IBusEngine*)rtk, text);
    
    ibus_rtk_engine_reset(rtk);
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

static void ibus_rtk_engine_lookup(IBusRTKEngine *rtk)
{
    IBusText *text;
    struct rtkresult *result;
    struct rtkinput *input;
    guint x;
    
    input = g_malloc_n(rtk->primitive_count, sizeof(struct rtkinput));
    for(x=0; x<rtk->primitive_count; x++)
        input[x].primitive = g_array_index(rtk->primitives, GString*, x)->str;
    
    rtk->lookup = result = rtk_lookup(rtk->primitive_count, input);
    
    if(!result)
    {
        ibus_rtk_engine_update_preedit(rtk, input);
        g_free(input);
        return;
    }
    
    g_free(input);
    
    ibus_lookup_table_clear(rtk->table);
    
    while(result->kanji)
    {
        text = ibus_text_new_from_printf("[%u] %s %s", result->number, result->kanji, result->meaning);
        ibus_lookup_table_append_candidate(rtk->table, text);
        result++;
    }
    
    ibus_rtk_engine_update_lookup(rtk);
}

static gboolean ibus_rtk_engine_process_key_event(IBusEngine *engine, guint keyval, guint keycode, guint modifiers)
{
    IBusRTKEngine *rtk = (IBusRTKEngine*)engine;
    GString *tmpstr;
    gboolean ret = rtk->preedit->len;
    guint x;
    
    if(modifiers)
    {
        if(modifiers & IBUS_RELEASE_MASK)
            return FALSE;
        
        if(modifiers & IBUS_SHIFT_MASK)
        {
            switch(keyval)
            {
            case IBUS_space:
            case IBUS_period:
            case IBUS_minus:
                goto input;
            default:
                if(is_alpha(keyval))
                    goto input;
            }
        }
        
        if(modifiers & IBUS_CONTROL_MASK)
        {
            switch(keyval)
            {
            case IBUS_w:
                if(rtk->primitive_cursor > 0)
                {
                    rtk->cursor -= rtk->primitive_cursor;
                    g_string_erase(rtk->preedit, rtk->cursor, rtk->primitive_cursor);
                    g_string_erase(primitive_current(0), 0, rtk->primitive_cursor);
                    rtk->primitive_cursor = 0;
                    ibus_rtk_engine_update_preedit(rtk, 0);
                }
                else if(rtk->primitive_current > 0)
                {
                    tmpstr = primitive_current(-1);
                    rtk->cursor -= tmpstr->len+1;
                    g_string_erase(rtk->preedit, rtk->cursor, tmpstr->len+1);
                    g_array_remove_index(rtk->primitives, rtk->primitive_current-1);
                    rtk->primitive_count--;
                    rtk->primitive_current--;
                    ibus_rtk_engine_update_preedit(rtk, 0);
                }
                break;
            case IBUS_u:
                g_string_erase(rtk->preedit, 0, rtk->cursor);
                rtk->cursor = 0;
                g_string_erase(primitive_current(0), 0, rtk->primitive_cursor);
                rtk->primitive_cursor = 0;
                g_array_remove_range(rtk->primitives, 0, rtk->primitive_current);
                rtk->primitive_count -= rtk->primitive_current;
                rtk->primitive_current = 0;
                ibus_rtk_engine_update_preedit(rtk, 0);
                break;
            case IBUS_a:
                goto home;
            case IBUS_e:
                goto end;
            case IBUS_Left:
                if(rtk->primitive_cursor > 0)
                {
                    rtk->cursor -= rtk->primitive_cursor;
                    rtk->primitive_cursor = 0;
                    ibus_rtk_engine_update_preedit(rtk, 0);
                }
                else if(rtk->primitive_current > 0)
                {
                    rtk->primitive_current--;
                    rtk->cursor -= primitive_current(0)->len+1;
                    ibus_rtk_engine_update_preedit(rtk, 0);
                }
                break;
            case IBUS_Right:
                tmpstr = primitive_current(0);
                if(rtk->primitive_cursor < tmpstr->len)
                {
                    rtk->cursor += tmpstr->len - rtk->primitive_cursor;
                    rtk->primitive_cursor = tmpstr->len;
                    ibus_rtk_engine_update_preedit(rtk, 0);
                }
                else if(rtk->primitive_current < rtk->primitive_count-1)
                {
                    rtk->cursor += tmpstr->len - rtk->primitive_cursor +1;
                    rtk->primitive_current++;
                    rtk->primitive_cursor = primitive_current(0)->len;
                    rtk->cursor += rtk->primitive_cursor;
                    ibus_rtk_engine_update_preedit(rtk, 0);
                }
                break;
            }
        }
        
        return ret;
    }
    
    switch(keyval)
    {
    case IBUS_Tab:
        if(rtk->prekanji->len)
        {
            ibus_lookup_table_cursor_down(rtk->table);
            ibus_rtk_engine_update_lookup(rtk);
        }
        else if(rtk->preedit->len)
            ibus_rtk_engine_lookup(rtk);
        break;
    case IBUS_Return:
        if(rtk->prekanji->len)
            ibus_rtk_engine_commit(rtk, rtk->prekanji);
        else
        {
            g_string_truncate(rtk->preedit, 0);
            if(rtk->primitive_count)
                 g_string_append(rtk->preedit,
                    g_array_index(rtk->primitives, GString*, 0)->str);
            for(x=1; x<rtk->primitive_count; x++)
                 g_string_append_printf(rtk->preedit, " %s",
                    g_array_index(rtk->primitives, GString*, x)->str);
            ibus_rtk_engine_commit(rtk, rtk->preedit);
        }
        break;
    case IBUS_Escape:
        if(rtk->preedit->len)
            ibus_rtk_engine_reset(rtk);
        break;
    case IBUS_BackSpace:
        if(rtk->preedit->len && rtk->cursor > 0)
        {
backspace:  rtk->cursor--;
            g_string_erase(rtk->preedit, rtk->cursor, 1);
            
            if(rtk->primitive_cursor > 0)
            {
                rtk->primitive_cursor--;
                g_string_erase(primitive_current(0), rtk->primitive_cursor, 1);
            }
            else
            {
                tmpstr = primitive_current(-1);
                rtk->primitive_cursor = tmpstr->len;
                g_string_append(tmpstr, primitive_current(0)->str);
                g_array_remove_index(rtk->primitives, rtk->primitive_current);
                rtk->primitive_count--;
                rtk->primitive_current--;
            }
            ibus_rtk_engine_update_preedit(rtk, 0);
        }
        break;
    case IBUS_Delete:
        if(rtk->preedit->len && rtk->cursor < rtk->preedit->len)
        {
            rtk->cursor++;
            if(rtk->primitive_cursor == primitive_current(0)->len)
            {
                rtk->primitive_current++;
                rtk->primitive_cursor = 0;
            }
            else
                rtk->primitive_cursor++;
            goto backspace;
        }
        break;
    case IBUS_Down:
        if(rtk->prekanji->len)
        {
            ibus_lookup_table_cursor_down(rtk->table);
            ibus_rtk_engine_update_lookup(rtk);
        }
        break;
    case IBUS_Up:
        if(rtk->prekanji->len)
        {
            ibus_lookup_table_cursor_up(rtk->table);
            ibus_rtk_engine_update_lookup(rtk);
        }
        break;
    case IBUS_Page_Down:
        if(rtk->prekanji->len)
        {
            ibus_lookup_table_page_down(rtk->table);
            ibus_rtk_engine_update_lookup(rtk);
        }
        break;
    case IBUS_Page_Up:
        if(rtk->prekanji->len)
        {
            ibus_lookup_table_page_up(rtk->table);
            ibus_rtk_engine_update_lookup(rtk);
        }
        break;
    case IBUS_Left:
        if(rtk->preedit->len && rtk->cursor > 0)
        {
            rtk->cursor--;
            ibus_rtk_engine_update_preedit(rtk, 0);
            
            if(rtk->primitive_cursor > 0)
                rtk->primitive_cursor--;
            else
            {
                rtk->primitive_current--;
                rtk->primitive_cursor = primitive_current(0)->len;
            }
        }
        break;
    case IBUS_Right:
        if(rtk->preedit->len && rtk->cursor < rtk->preedit->len)
        {
            rtk->cursor++;
            ibus_rtk_engine_update_preedit(rtk, 0);
            
            if(rtk->primitive_cursor < primitive_current(0)->len)
                rtk->primitive_cursor++;
            else
            {
                rtk->primitive_current++;
                rtk->primitive_cursor = 0;
            }
        }
        break;
    case IBUS_space:
        if(!rtk->preedit->len || !rtk->cursor)
            break;
        g_string_insert_c(rtk->preedit, rtk->cursor, IBUS_period);
        rtk->cursor++;
        ibus_rtk_engine_update_preedit(rtk, 0);
        
        tmpstr = g_string_new("");
        g_array_insert_val(rtk->primitives, rtk->primitive_current+1, tmpstr);
        rtk->primitive_count++;
        
        tmpstr = primitive_current(0);
        if(rtk->primitive_cursor < tmpstr->len)
        {
            g_string_assign(primitive_current(+1), tmpstr->str+rtk->primitive_cursor);
            g_string_truncate(tmpstr, rtk->primitive_cursor);
        }
        rtk->primitive_current++;
        rtk->primitive_cursor = 0;
        return TRUE;
    case IBUS_Home:
home:   rtk->cursor = 0;
        rtk->primitive_current = 0;
        rtk->primitive_cursor = 0;
        ibus_rtk_engine_update_preedit(rtk, 0);
        break;
    case IBUS_End:
end:    rtk->cursor = rtk->preedit->len;
        rtk->primitive_current = rtk->primitive_count-1;
        rtk->primitive_cursor = primitive_current(0)->len;
        ibus_rtk_engine_update_preedit(rtk, 0);
        break;
    case IBUS_period:
    case IBUS_minus:
        goto input;
    default:
        if(is_alpha(keyval))
        {
input:      g_string_insert_c(rtk->preedit, rtk->cursor, keyval);
            rtk->cursor++;
            g_string_insert_c(primitive_current(0), rtk->primitive_cursor, keyval);
            rtk->primitive_cursor++;
            ibus_rtk_engine_update_preedit(rtk, 0);
            return TRUE;
        }
    }
    
    return ret;
}

static void ibus_rtk_engine_focus_in(IBusEngine *engine)
{
    ibus_rtk_engine_update_preedit((IBusRTKEngine*)engine, 0);
}
