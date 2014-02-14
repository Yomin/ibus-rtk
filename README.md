# ibus-rtk

RTK engine for IBus.
ibus-rtk implements Kanji search by Heisig primitives.
For more information about Heisig primitives use the search engine
of your choice and search for "James Heisig Remembering The Kanji".

## activate

ibus-rtk is found in the IBus Input Method list below Japanese.

## usage

### keys

 mod   | key    | info
:-----:|:------:|------
       | space  | start next primitive
 shift | space  | insert space into floating text
       | tab    | lookup Kanjis by primitives<br />set floating text to first found kanji<br />display lookup table to choose other Kanjis from<br />if lookup table already visible same as down key
       | return | commit current floating text
       | down   | choose next kanji from lookup table
       | up     | choose previous kanji from lookup table
       | left   | move cursor left through floating text
       | right  | move cursor right through floating text
       | home   | move cursor to begin of floating text
       | end    | move cursor to end of floating text
 ctrl  | left   | move cursor to primitive begin or begin of left primitive
 ctrl  | right  | move cursor to primitive end or begin of right primitive
 ctrl  | a      | same as home key
 ctrl  | e      | same as end key
 ctrl  | w      | remove floating text to primitive begin or begin of left primitive
 ctrl  | u      | remove floating text to begin
       | *      | insert into floating text or ignore

### input

Every non special purpose or ignored character is inserted into
floating text and additionally underlined. If 'space' is used to
start a new primitive a not underlined 'period' is inserted into
floating text. Normal periods will be underlined.

### lookup

If the lookup is successful the floating text is set to the
first found Kanji.
If the lookup failed every found primitive is colored green and
every unfound primitive is colored red.

## credits

This IBus engine is derived from Peng Huangs ibus-tmpl template engine.

