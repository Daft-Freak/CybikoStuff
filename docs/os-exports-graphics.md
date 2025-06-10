# Info about CyOS exports that nobody asked for

## BaseBitmap

```c++
struct BaseBitmap
{
    asserts _asserts;

    int unk;
    int _padding; //?
    int x;
    int y;
    int w;
    int h;
    char *buf;
    int bpp;
    bool allocated;
    char _padding2;
};
```

`__cyos 84 void BaseBitmap_dtor(BaseBitmap *, int)`

`__cyos 171 BaseBitmap *BaseBitmap_ctor(BaseBitmap *)`

`__cyos 172 BaseBitmap *BaseBitmap_ctor_Ex1(BaseBitmap *, int w, int h, int bpp)`

`__cyos 173 BaseBitmap *BaseBitmap_ctor_Ex2(BaseBitmap *, int w, int h, int bpp, char *buf)`

`__cyos 275 void *BaseBitmap_vtable`

`__cyos 345 bool BaseBitmap_allocate(BaseBitmap *, int w, int h, int bpp)`

`__cyos 346 int BaseBitmap_get_data_size(BaseBitmap *)`

`__cyos 347 void BaseBitmap_free(BaseBitmap *)`

`__cyos 349 void BaseBitmap_free_data(BaseBitmap *, void *)`

`__cyos 351 void *BaseBitmap_alloc_data(BaseBitmap *, long)`

`__cyos 397 int BaseBitmap_calc_data_size(BaseBitmap *, int w, int h, int bpp)`

`__cyos 419 char *BaseBitmap_class_name(BaseBitmap *)`

Returns "BaseBitmap".

`__cyos 475 void BaseBitmap_copy(BaseBitmap *src, BaseBitmap *dst)`

`__cyos 772 bool BaseBitmap_init(BaseBitmap *)`

`__cyos 840 bool BaseBitmap_load(BaseBitmap *, Input *)`

`__cyos 1193 bool BaseBitmap_store(BaseBitmap *, Output *)`

## Bitmap

```c++
struct Bitmap : public BaseBitmap
{
    int width;
    int height;
};

```

### In SDK

`__cyos 106 void Bitmap_dtor(Bitmap *ptr_bitmap, int memory_flag)`

`__cyos 217 Bitmap *Bitmap_ctor(Bitmap *ptr_bitmap)`

`__cyos 218 Bitmap *Bitmap_ctor_Ex1(Bitmap *ptr_bitmap, char *filename)`

`__cyos 219 Bitmap *Bitmap_ctor_Ex3(Bitmap *ptr_bitmap, Bitmap *templ)`

`__cyos 220 Bitmap *Bitmap_ctor_Ex2(Bitmap *ptr_bitmap, int width, int height, int bpp)`

`__cyos 430 char *Bitmap_class_name(Bitmap *_0)`

`__cyos 440 void Bitmap_clear(Bitmap *ptr_bitmap)`

`__cyos 584 void Bitmap_fill(Bitmap *ptr_bitmap, color_t color)`

`__cyos 677 int Bitmap_get_height(Bitmap *ptr_bitmap)`

`__cyos 713 rect_t *Bitmap_get_r(Bitmap *ptr_bitmap, rect_t *rc)`

`__cyos 714 rect_t *Bitmap_get_rect(Bitmap *ptr_bitmap, rect_t *rc)`

`__cyos 750 int Bitmap_get_width(Bitmap *ptr_bitmap)`

`__cyos 846 bool Bitmap_load(Bitmap *ptr_bitmap, Input *input)`

`__cyos 1195 bool Bitmap_store(Bitmap *ptr_bitmap, Output *output)`

### Undocumented

`get_x/y/w/h` are possibly `BaseBitmap_...`.

`__cyos 311 void *Bitmap_vtable`

`__cyos 476 Bitmap_copy(BaseBitmap *src, BaseBitmap *dst)`

`__cyos 676 int Bitmap_get_h(Bitmap *)`

Same as `Bitmap_get_r(bmp->).h`/`bmp->h`, not `bmp->height`

`__cyos 748 int Bitmap_get_w(Bitmap *)`

`__cyos 752 int Bitmap_get_x(Bitmap *)`

`__cyos 754 int Bitmap_get_y(Bitmap *)`

`__cyos 787 bool Bitmap_init(Bitmap *)`

## BitmapSequence

```c++
struct BitmapSequence : public Bitmap
{
    void *heap;
    long heap_size;
    long heap_offset;
    SeqBmp *frames;
    int num_frames;
    int _padding
};
```

### In SDK

`__cyos 96 void BitmapSequence_dtor(BitmapSequence *ptr_bitmap_sequence, int memory_flag)`

`__cyos 192 BitmapSequence *BitmapSequence_ctor(BitmapSequence *ptr_bitmap_sequence)`

`__cyos 193 BitmapSequence *BitmapSequence_ctor_Ex(BitmapSequence *ptr_bitmap_sequence, char *file_name)`

`__cyos 424 char *BitmapSequence_class_name(BitmapSequence *ptr_bitmap_sequence)`

`__cyos 634 Bitmap *BitmapSequence_get_bitmap(BitmapSequence *ptr_bitmap_sequence, int bitmap_index)`

`__cyos 727 int BitmapSequence_get_size(BitmapSequence *ptr_bitmap_sequence)`

`__cyos 751 int BitmapSequence_get_x(BitmapSequence *ptr_bitmap_sequence, int bitmap_index)`

`__cyos 753 int BitmapSequence_get_y(BitmapSequence *ptr_bitmap_sequence, int bitmap_index)`

`__cyos 842 bool BitmapSequence_load(BitmapSequence *ptr_bitmap_sequence, Input *input)`

`__cyos 1194 bool BitmapSequence_store(BitmapSequence *ptr_bitmap_sequence, Output *output)`

### Undocumented

`__cyos 294 void *BitmapSequence_vtable`

`__cyos 353 void *BitmapSequence_alloc_image(BitmapSequence *, long size)`

`__cyos 781 bool BitmapSequence_init(BitmapSequence *)`

`__cyos 799 bool BitmapSequence_in_range(BitmapSequence *, int)`

`__cyos 857 bool BitmapSequence_load_resource(BitmapSequence *, char *)`

## Font

``` c++
struct Font : public BitmapSequence
{
    char name[32];
    bool fixed;
    char _padding;
    int char_height;
    int spacing;
    int _padding2;
};
```

### In SDK

`__cyos 102 void Font_dtor(Font *ptr_font, int memory_flag)`

`__cyos 210 Font *Font_ctor(Font *ptr_font)`

`__cyos 211 Font *Font_ctor_Ex(Font *ptr_font, char *file_name, bool fixed, int spacing)`

`__cyos 387 Bitmap *Font_bmp_by_char(Font *ptr_font, int chr)`

`__cyos 428 char *Font_class_name(Font *ptr_font)`

`__cyos 649 int Font_get_char_height(Font *ptr_font)`

`__cyos 651 int Font_get_char_width(Font *ptr_font, int chr)`

`__cyos 691 char *Font_get_name(Font *ptr_font)`

`__cyos 730 int Font_get_spacing(Font *ptr_font)`

`__cyos 736 int Font_string_width(Font *ptr_font, char *str)`

`__cyos 737 int Font_string_width_Ex(Font *ptr_font, char *str, int len)`

`__cyos 809 bool Font_is_fixed(Font *ptr_font)`

`__cyos 844 bool Font_load(Font *ptr_font, Input *input)`

`__cyos 1123 void Font_set_fixed(Font *ptr_font, bool fixed)`

`__cyos 1135 void Font_set_name(Font *ptr_font, char *filename)`

`__cyos 1155 void Font_set_spacing(Font *ptr_font, int spacing)`

`__cyos 1169 char *Font_split_string(Font *ptr_font, char *str, int width, int *len)`

### Undocumented

`__cyos 305 void *Font_vtable`

`__cyos 1220 Font *default_font`

Set to the first font that loads successfully and used as a fallback for the other system fonts.

## SeqBmp

``` c++
struct SeqBmp : public Bitmap
{
    BitmapSequence *sequence;
};
```

`__cyos 111 void SeqBmp_dtor(SeqBmp *, int)`

`__cyos 228 SeqBmp *SeqBmp_ctor(SeqBmp *)`

`__cyos 319 void *SeqBmp_vtable`

`__cyos 350 void SeqBmp_free_data(SeqBmp *, void *)`

`__cyos 352 void *SeqBmp_alloc_data(SeqBmp *, long)`

`__cyos 431 char *SeqBmp_class_name(SeqBmp *)`

`__cyos 849 bool SeqBmp_load(SeqBmp *, Input *)`

## Display

```c++
struct Display
{
    bool unk; // skips update if true
    char _padding;
    int width;
    int height;
    int bpp;
    char unk2[10];
    char front_buf[4000];
    char back_buf[4000];
    char sys_buf[400];
    int _padding2;
    void *unk_fun; // called in update
};
```

`__cyos 206 Display *Display_ctor(Display *)`

`__cyos 785 void Display_init(Display *)`

`__cyos 833 Display *display_ptr`

`__cyos 1018 void Display_update(Display *, int buf)`

`__cyos 1110 void Display_set_contrast(Display *, int)`

`__cyos 1138 void Display_set_palette(Display *, int index, int value)`

`__cyos 1139 void Display_set_params(Display *, int *params)`

`__cyos 1149 void Display_set_reversed(Display *, bool)`

Some others: 946, 1017, 1019, 1500, 1689.

## TGraph

```c++
struct TGraph
{
    color_t color;
    color_t bkcolor;
    int mapped_color;
    int mapped_bkcolor;
    int draw_mode;
    int byte_stride;
    char *_buf;
    int unk;  // index 662 gets these
    int unk2; // index 1111 sets them
    int width;
    int height;
    int clip_x;
    int clip_y;
    int clip_x2;
    int clip_y2;
};
```

### In SDK

`__cyos 112 void TGraph_dtor(TGraph *ptr_graph, int)`

`__cyos 528 void TGraph_draw_hline(TGraph *ptr_graph, int x, int y, int dx)`

`__cyos 530 void TGraph_draw_line(TGraph *ptr_graph, int x1, int y1, int x2, int y2)`

`__cyos 534 void TGraph_draw_vline(TGraph *ptr_graph, int x, int y, int dy)`

`__cyos 587 void TGraph_fill_screen(TGraph *ptr_graph, color_t fc)`

`__cyos 645 int TGraph_get_bytes_total(TGraph *ptr_graph)`

`__cyos 655 void TGraph_get_clip(TGraph *ptr_graph, rect_t *rc)`

`__cyos 709 color_t TGraph_get_pixel(TGraph *ptr_graph, int x, int y)`

`__cyos 966 void TGraph_put_background(TGraph *ptr_graph, char *ptr_background)`

`__cyos 1057 void TGraph_scroll(TGraph *ptr_graph, int left, int top, int width, int height, int dx, int dy)`

`__cyos 1098 void TGraph_set_bkcolor(TGraph *ptr_graph, color_t color)`

`__cyos 1103 void TGraph_set_clip_Ex(TGraph *ptr_graph, rect_t *rc)`

`__cyos 1104 void TGraph_set_clip(TGraph *ptr_graph, int x, int y, int width, int height)`

`__cyos 1108 void TGraph_set_color(TGraph *ptr_graph, color_t color)`

`__cyos 1142 void TGraph_set_pixel(TGraph *ptr_graph, int x, int y, color_t color)`

### Undocumented

`__cyos 229 TGraph *TGraph_ctor(TGraph *)`

`__cyos 445 void TGraph_clear_screen(TGraph *)`

Fills with 0.

`__cyos 449 bool TGraph_clip(TGraph *, int *, int *, int *, int *)`

`__cyos 529 void TGraph_draw_image(TGraph *, int x, int y, char *buf, int w, int h, int mode)`

`__cyos 531 void TGraph_draw_rect(TGraph *, int left, int top, int right, int bottom)`

Rect outline.

`__cyos 586 void TGraph_fill_rect(TGraph *, int left, int top, int right, int bottom)`

`__cyos 788 void TGraph_init(TGraph *)`

`__cyos 1115 void TGraph_set_buffer(TGraph *, char *buf, int w, int h)`


Others include 483, 620, 662, 764, 874, 972, 1058, 1111, 1227, 1228, 1229, 1230.

## Graphics

```c++
struct Graphics : public TGraph
{
    asserts _asserts;
    BaseBitmap *bitmap;
    Font *font;
};
```

### In SDK

`__cyos 243 Graphics *Graphics_ctor(Graphics *ptr_gfx)`

`__cyos 244 Graphics *Graphics_ctor_Ex(Graphics *ptr_gfx, Bitmap *bitmap)`

`__cyos 435 char *Graphics_class_name(Graphics *ptr_gfx)`

`__cyos 526 void Graphics_draw_bitmap(Graphics *ptr_gfx, Bitmap *bmp, int left, int top, short fm)`

`__cyos 527 int Graphics_draw_char(Graphics *ptr_gfx, int x, int y, char fc)`

`__cyos 532 void Graphics_draw_text(Graphics *ptr_gfx, char *text, int left, int top)`

`__cyos 533 void Graphics_draw_text_Ex(Graphics *ptr_gfx, char *str, int left, int top, int len)`

`__cyos 635 Bitmap *Graphics_get_bitmap(Graphics *ptr_gfx)`

`__cyos 650 int Graphics_get_char_height(Graphics *ptr_gfx)`

`__cyos 652 int Graphics_get_char_width(Graphics *ptr_gfx, char chr)`

`__cyos 673 Font *Graphics_get_font(Graphics *ptr_gfx)`

`__cyos 738 int Graphics_string_width(Graphics *ptr_gfx, char *str)`

`__cyos 739 int Graphics_string_width_Ex(Graphics *ptr_gfx, char *str, int len)`

`__cyos 1097 void Graphics_set_bitmap(Graphics *ptr_gfx, Bitmap *bmp)`

`__cyos 1126 void Graphics_set_font(Graphics *ptr_gfx, Font *font)`

## DisplayGraphics

```c++
struct DisplayGraphics : public Graphics
{
    Display *display;
    int cur_page;
    int _padding;
    BaseBitmap *other_bitmap;
};
```

### In SDK

`__cyos 425 char *DisplayGraphics_class_name(DisplayGraphics*)`

`__cyos 599 void DisplayGraphics_flip(DisplayGraphics *ptr_gfx)`

`__cyos 702 int DisplayGraphics_get_page(DisplayGraphics *ptr_gfx)`

`__cyos 703 char *DisplayGraphics_get_page_ptr(DisplayGraphics *ptr_gfx, int page)`

`__cyos 934 void DisplayGraphics_page_copy_Ex(DisplayGraphics *ptr_gfx, int from_page, int to_page, rect_t *rc)`

`__cyos 935 void DisplayGraphics_page_copy(DisplayGraphics *ptr_gfx, int from_page, int to_page, int x, int y, int w, int h)`

`__cyos 1137 void DisplayGraphics_set_page(DisplayGraphics *ptr_gfx, int page)`

`__cyos 1163 void DisplayGraphics_show(DisplayGraphics *ptr_gfx)`

`__cyos 1164 void DisplayGraphics_show_Ex(DisplayGraphics *ptr_gfx, int page)`


### Undocumented

`__cyos 36 DisplayGraphics *display_graphics`

The global instance.

`__cyos 196 DisplayGraphics *DisplayGraphics_ctor(DisplayGraphics *)`

`__cyos 782 void DisplayGraphics_init(DisplayGraphics *)`

`__cyos 1130 void DisplayGraphics_set_display(DisplayGraphics *, Display *)`

## DisplayPage

```c++
struct DisplayPage : public Graphics
{
    // no extra members
};
```

`__cyos 88 void DisplayPage_dtor(DisplayPage *, int)`

`__cyos 179 DisplayPage *DisplayPage_ctor(DisplayPage *)`

`__cyos 422 char *DisplayPage_class_name(DisplayPage *)`

## SysGraphics

```c++
struct SysGraphics : public Graphics
{
    // no extra members
};
```

`__cyos 58 SysGraphics *sys_graphics`

`__cyos 198 SysGraphics *SysGraphics_ctor(SysGraphics *)`

`__cyos 426 char *SysGraphics_class_name(SysGraphics *)`

`__cyos 784 void SysGraphics_init(SysGraphics *)`

`__cyos 1131 void SysGraphics_set_display(SysGraphics *, Display *)`