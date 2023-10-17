

These are bash scripts used to play a pronunciation of selected Japanese text from a local database and copy the audio file to the system clipboard.
I tried to minimize the latency as much as possible. The exection speed is about 0.01-0.07s on my low end system.\
\
pronunciation-cli searches through the search_order directories for the selected text and the hiragana conversion and plays the first match. If nothing was found
it falls back to non-exact searches, so the pronunciation can also contain more than the searched text\
\
pronunciation-cli-all uses dmenu to give a list of non-exact matches, which could also be used to find voiced example sentences

## Dependencies
locate (any implementation), ffmpeg, yomi binary in PATH (see https://github.com/GenjiFujimoto/mecab-tools)

## Expected File format
To enable a fast search, it expects the name of each audio file to be enclosed with dots (eg. audio_dir/.猿.mp3).
The reason for this was that the Japanese audio files I used, where in the format 'word_reading.ogg' (eg. 簡単_かんたん.ogg), so I used some sed
to turn them into '.word.reading.ogg' which enables search for reading or word without any use of regular expressions.

## Database setup
The database has to be built with `updatedb -U <folder with pronunciations> -o pronunciations.db`. The path to the resulting file needs to be specified in the
file.

## Config
At the beginning of the script you can specify your order of prefrences for different pronunciation sources (which is the name of the directory).

## Usage
Execute `pronunciation-cli`. This uses the primary-seletion, so ideally one would bind this to a key.
