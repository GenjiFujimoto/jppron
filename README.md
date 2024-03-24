This is a work in progress application to play a pronunciation of a Japanese word, using a local database
in the [ajatt audio format](https://github.com/Ajatt-Tools/nhk_2016_pronunciations_index).

## Installation
`sudo make install`

## Usage
`jppron word [reading]`. The very first run might take a while, since it will create an index saved in 
`$XDG_DATA_HOME/jppron/`.
