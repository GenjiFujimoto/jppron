

Work in progress C rewrite of the script. Made for the index format as in https://github.com/Ajatt-Tools/nhk_2016_pronunciations_index.
Currently plays all the files of the first exact word match and resorts back to an exact kana match if nothing found.

To install this program, first change the folder to the pronunciations in
`jppron` according to your setup. The `search_depth` variable is the depth of
subfolders the search descents into. Then install with `make && sudo make install`.
