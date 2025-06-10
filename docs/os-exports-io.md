# Info about CyOS exports that nobody asked for


`Input`/`Output` subclasses always end with this and the first member points to it.

```c++
struct unk_io
{
    int flags;
    int _padding;
    void *unk;
    void *vtable;
};
```

## FileBase

```c++
struct FileBase : public asserts
{
    int unk;
    int _padding;
    unk_io *unk2;
    void *io_struct; // IO goes through this thing
    long unk3;
    Input *input; // used for redirecting to flash data
    char *filename;
    char unk4[12];
    void *file_handle; // handle for file info
    int offset;
    char unk5[2]
    int block_index_in_file;
    bool opened;
    bool writing;
    char unk6; // an error code?
    char unk7[3]; // padding?
    long size;
    // unk_io here if not subclassed
};
```

`__cyos 117 FileBase_dtor(FileBase *, int)`

`__cyos 239 FileBase *FileBase_ctor(FileBase *, int)`

`__cyos 240 FileBase *FileBase_ctor_Ex(FileBase *, int, char *, bool read_only, bool create)`

`__cyos 327 void *FileBase_vtable`

`__cyos 433 char *FileBase_class_name(FileBase *)`

`__cyos 453 void FileBase_close(FileBase *)`

`__cyos 641 int FileBase_get_byte(FileBase *)`

`__cyos 644 void FileBase_get(FileBase *, void *buf, long size)`

`__cyos 725 long FileBase_get_size(FileBase *)`

`__cyos 790 void FileBase_init(FileBase *)`?

Calls the next one.

`__cyos 797 void FileBase_init(FileBase *)`?

Called by above, one of these is the right one...

`__cyos 913 bool FileBase_open(FileBase *, char *, bool read_only, bool create)`

`__cyos 968 void FileBase_put_byte(FileBase *, int)`

`__cyos 969 void FileBase_put(FileBase *, void *buf, long size)`

Others include 400, 401, 486, 502, 601, 697, 1113.

## Input

```c++
struct Input
{
    unk_io *unk;
    void *vtable;
    // unk_io here if not subclassed
};
```

Input vtable is:
```c++
struct Input_vtable
{
    void *_0;
    void *_1;

    long (**read)(Input *, void *buffer, long length);
    int (**read_byte)(Input *);
    long (**seek)(Input *, long pos, seek_t mode);
    long (**tell)(Input *);
};
```

### In SDK

The SDK defines a bunch of other functions, but not as OS exports.

`__cyos 1001 long Input_read_dword(Input* ptr_input)`

`__cyos 1007 short Input_read_word(Input* ptr_input)`

### Undocumented

`__cyos 103 void Input_dtor(Input *, int)`

`__cyos 306 void *Input_vtable`

`__cyos 803 bool Input_is_bad(Input *)`

`__cyos 808 bool Input_is_eof(Input *)`

`__cyos 811 bool Input_is_good(Input *)`

`__cyos 845 long Input_load(Input *, void *, long)`

This just calls `read` through the vtable, but it's in between a bunch of functions called "load" in the export table...

`__cyos 1063 long Input_seek(Input *, long pos, seek_t mode)`

`__cyos 1238 long Input_tell(Input *)`

## ConsoleInput

Reads from OS console.

```c++
struct ConsoleInput : public Input
{
    bool unk;
    char _padding[3];
    Input *parent; // connected to a pipe
    Mutex mutex;
    // unk_io here if not subclassed
};
```

`__cyos 91 void ConsoleInput_dtor(ConsoleInput *, int)`

`__cyos 286 void *ConsoleInput_vtable`

`__cyos 985 long ConsoleInput_read(ConsoleInput *, void *, long)`

`__cyos 996 int ConsoleInput_read_byte(ConsoleInput *)`

## ConLineInput

Wrapper around `ConsoleInput` and `ConsoleOutput` with some extra helpers.

```c++
struct ConLineInput : public Input, public Output
{
    ConsoleInput *input;
    ConsoleOutput *output;
    Mutex mutex;
    // unk_io here if not subclassed
};
```

`__cyos 684 int ConLineInput_read_string(ConLineInput *, char *, long)`

`__cyos 1231 ConsoleLineInput *console_line_input_ptr`

`__cyos 1480 void ConLineInput_set_timeout(ConLineInput *, clock_t)`

## DecompressionInput

Used for reading compressed archive entries.

```c++
// related to some rom exports...
struct BMCState
{
    char *src_data;
    long src_offset;
    int cur_data_byte;
    int _padding;
};

// also has some exports...
struct Decompressor
{
    BMCState state;
    long decompressed_len;
    long decompressed_offset;
    char decompressed_data[8192];
    char compressed_data[64];
    long unk;
    void *vtable;
};
```

```c++
struct DecompressionInput : public Input, public Decompressor
{
    Input *parent;
    long decompressed_size;
    // unk_io here if not subclassed
};
```

`__cyos 236 DecompressionInput *DecompressionInput_ctor(DecompressionInput *, int, Input *)`

`__cyos 324 void *DecompressionInput_vtable`

`__cyos 724 long DecompressionInput_get_size(DecompressionInput *)`

`__cyos 987 long DecompressionInput_read(DecompressionInput *, void *, long)`

`__cyos 998 int DecompressionInput_read_byte(DecompressionInput *)`


## FileInput

```c++
struct FileInput : public Input
{
    FileBase file; // not entirely figured out (60 bytes)
    long offset;
    // unk_io here if not subclassed
};
```

### In SDK

`__cyos 250 FileInput* FileInput_ctor_STUB(FileInput* ptr_file_input, int Magic)`

`__cyos 251 FileInput* FileInput_ctor_Ex_STUB(FileInput* ptr_file_input, int Magic, char* file_name)`

`__cyos 436 char* FileInput_class_name(FileInput* ptr_file_input)`

`__cyos 914 bool FileInput_open(FileInput* ptr_file_input, char* sz_file_name)`

`__cyos 1061 long FileInput_seek(FileInput* ptr_file_input, long pos, seek_t mode)`

`__cyos 1236 long FileInput_tell(FileInput* ptr_file_input)`

### Undocumented

`__cyos 119 void FileInput_dtor(FileInput *, int)`

`__cyos 334 void *FileInput_vtable`

`__cyos 726 long FileInput_get_size(FileInput *)`

`__cyos 792 void FileInput_init(FileInput *)`

`__cyos 988 long FileInput_read(FileInput *, void *, long)`

`__cyos 999 int FileInput_read_byte(FileInput *)`

`__cyos 1064 long FileInput_seek_(FileInput *, long, seek_t)`

The one in the vtable, calls the other one.

`__cyos 1143 long FileInput_set_offset(FileInput *, long)`

Validates and stores the offset, but doesn't seek.

`__cyos 1239 long FileInput_tell_(FileInput *)`

The one in the vtable.

## SubsetInput

Wraps an `Input` and limits to a range of it. Used for reading archive entries

``` c++
struct SubsetInput : public Input
{
    long parent_offset;
    long parent_size;
    long offset;
    Input *parent;
    // unk_io here if not subclassed
};
```

`__cyos 90 void SubsetInput_dtor(SubsetInput *, int)`

`__cyos 185 SubsetInput *SubsetInput_ctor(SubsetInput *, int, long offset, long size)`

`__cyos 284 void *SubsetInput_vtable`

`__cyos 721 long SubsetInput_get_size(SubsetInput *)`

`__cyos 984 long SubsetInput_read(SubsetInput *, void *, long)`

`__cyos 995 int SubsetInput_read_byte(SubsetInput *)`

`__cyos 1062 long SubsetInput_seek(SubsetInput *, long, seek_t)`

`__cyos 1222 void SubsetInput_update_eof(SubsetInput *)`

`__cyos 1237 long SubsetInput_tell(SubsetInput *)`

## TextInput

Wrapper around an import with extra text-related helpers.

``` c++
struct TextInput : public Input
{
    Input *parent;
    // unk_io here if not subclassed
};
```

`__cyos 110 void TextInput_dtor(TextInput *, int)`

`__cyos 227 TextInput *TextInput_ctor(TextInput *, int, Input *)`

`__cyos 317 void *TextInput_vtable`

`__cyos 986 long TextInput_read(TextInput *, void *, long)`

`__cyos 997 int TextInput_read_byte(TextInput *)`

`__cyos 1000 int TextInput_read_char(TextInput *)`

Like `_read_byte`, but fails if the byte isn't ascii.

`__cyos 1002 char *TextInput_read_line(TextInput *, char *, long)`


## Output


```c++
struct Output
{
    unk_io *unk;
    void *vtable;
    // unk_io here if not subclassed
};
```

Input vtable is:
```c++
struct Output_vtable
{
    void *_0;
    void *_1;

    long (**write)(Output *, void *, long);
    int (**write_byte)(Output *, int);
    long (**seek)(Output *, long, seek_t);
    long (**tell)(Output *);
};
```

### In SDK

The SDK defines a bunch of other functions, but not as OS exports.

`__cyos 1293 long Output_write_dword(Output* ptr_output, long dword)`

`__cyos 1298 short Output_write_word(Output* ptr_output, short word)`

### Undocumented

`__cyos 109 void Output_dtor(Output *, int)`

`__cyos 315 void *Output_vtable`

`__cyos 1052 long Output_write(Output *, void *, long)`

Calls `write` through vtable.

`__cyos 1066 long Output_seek(Output *, long, seek_t)`

`__cyos 1241 long Output_tell(Output *)`

## ConsoleOutput

Writes to OS console.

```c++

struct ConsoleOutput : public Output
{
    bool unk;
    char _padding[3];
    Output *parent; // connected to a pipe
    Output *parent2; // used as a fallback for ^?
    MutexCounter mutex;
    // unk_io here if not subclassed
};
```

`__cyos 95 void ConsoleOutput_dtor(ConsoleOutput *, int)`

`__cyos 291 void *ConsoleOutput_vtable`

`__cyos 1284 long ConsoleOutput_write(ConsoleOutput *, void *, long)`

`__cyos 1289 int ConsoleOutput_write_byte(ConsoleOutput *, int)`

## FileOutput


```c++
struct FileOutput : public Output
{
    FileBase file; // not entirely figured out (60 bytes)
    long offset;
    // unk_io here if not subclassed
};
```

### In SDK

`__cyos 174 FileOutput* FileOutput_ctor_STUB(FileOutput* ptr_file_output, int Magic)`

`__cyos 175 FileOutput* FileOutput_ctor_Ex_STUB(FileOutput* ptr_file_output, int Magic, char* filename, bool create, short attr)`

`__cyos 420 char* FileOutput_class_name(FileOutput* ptr_file_output)`

`__cyos 912 bool FileOutput_open_STUB(FileOutput* ptr_file_output, char* file_name, bool create, short attr)`

`__cyos 1060 long FileOutput_seek(FileOutput* ptr_file_output, long pos, seek_t mode)`

`__cyos 1235 long FileOutput_tell(FileOutput* ptr_file_output)`

`__cyos 1257 bool FileOutput_truncate(FileOutput* ptr_file_output, long position)`

### Undocumented

`__cyos 85 void FileOutput_dtor(FileOutput *, int)`

`__cyos 276 void *FileOutput_vtable`

`__cyos 720 long FileOutput_get_size(FileOutput *)`

`__cyos 774 void FileOutput_init(FileOutput *)`

`__cyos 1065 long FileOutput_seek_(FileOutput *, long, seek_t)`

`__cyos 1240 long FileOutput_tell_(FileOutput *)`

`__cyos 1282 long FileOutput_write(FileOutput *, void *, long`

`__cyos 1791 long FileOutput_read(FileOutput *, void *, long)`

No, really.

`__cyos 1287 int FileOutput_write_byte(FileOutput *, int)`


## SubsetOutput

Wraps an `Output` and limits to a range of it. Used for writing archive entries

``` c++
struct SubsetOutput : public Output
{
    long parent_offset;
    long parent_size;
    long offset;
    Output *parent;
    // unk_io here if not subclassed
};
```

`__cyos 1329 void SubsetOutput_dtor(SubsetOutput *, int)`

`__cyos 1330 void *SubsetOutput_vtable`

`__cyos 1332 long SubsetOutput_get_size(SubsetOutput *)`

`__cyos 1335 long SubsetOutput_seek(SubsetOutput *, long, seek_t)`

`__cyos 1136 void SubsetOutput_update_eof(SubsetOutput *)`

`__cyos 1137 long SubsetOutput_tell(SubsetOutput *)`

`__cyos 1138 long SubsetOutput_write(SubsetOutput *, void *, long)`

`__cyos 1139 int SubsetOutput_write_byte(SubsetOutput *, int)`


## TextOutput

Like `TextInput` but an `Output`... probably.

```c++
struct TextOutput : public Output
{
    Output *parent;
    // unk_io here if not subclassed
};
```

`__cyos 113 void TextOutput_dtor(TextOutput *, int)`

`__cyos 232 TextOutput *TextOutput_ctor(TextOutput *, int, Output *)`

`__cyos 320 TextOutput_vtable`

`__cyos 1285 long TextOutput_write(TextOutput *, void *, long)`

`__cyos 1290 int TextOutput_write_byte(TextOutput *, int)`

`__cyos 1291 bool TextOutput_write_char(TextOutput *, int)`

`__cyos 1294 char *TextOutput_write_line(TextOutput *, char *, bool newline)`