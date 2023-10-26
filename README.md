

Work in progress application to play a pronunciation of the selected text provided by a local database. Made for the index format as in https://github.com/Ajatt-Tools/nhk_2016_pronunciations_index.
Currently plays all the files of the first exact word match and falls back to an exact kana match if nothing found.

## Installation
First change the folder to the pronunciations in
`jppron` according to your setup. The `search_depth` variable is the depth of
subfolders the search descents into. Then install with `make && sudo make install`.

## Usage
`jppron [word]` if no word is provided, the selection will be used.
