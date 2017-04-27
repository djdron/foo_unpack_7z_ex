# foo_unpack_7z_ex
7-Zip unpacker plugin for foobar2000 based on C++ LZMA SDK

When i did massive testing of foo_input_zxtune plugin i've spotted some problem - foo_unpack_7z is unable to process huge solid archives.
I got them from modland.torrent. Some of them are ~14Gb size.
But even for archives with size ~200 Mb it fails.
After source code investigation & debugging i noticed that plugin call C LZMA SDK API which tries to allocate more than 2 Gb of RAM to extract files.
And 32-bit foobar2000 process failed to do this. 
After googling i found this limitation of C LZMA SDK.
7-Zip author recommend to use C++ LZMA SDK API.
So foo_unpack_7z_ex plugin was born.
It succesfully parse all modland.torrent 7z archives.
