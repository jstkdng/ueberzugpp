#!/bin/sh

image() {
    FILE_PATH="$1"
    X=$4
    Y=$5
    MW=$(($2 - 1))
    MH=$3
    ueberzugpp cmd -s "$UB_SOCKET" -a add -i PREVIEW -x "$X" -y "$Y" --max-width "$MW" --max-height "$MH" -f "$FILE_PATH"
    exit 1
}

batorcat() {
    file="$1"
    shift
    if command -v bat >/dev/null 2>&1; then
        bat --color=always --style=plain --pager=never "$file" "$@"
    else
        cat "$file"
    fi
}

CACHE="$HOME/.cache/lf/thumbnail.$(stat --printf '%n\0%i\0%F\0%s\0%W\0%Y' -- "$(readlink -f "$1")" | sha256sum | awk '{print $1}'))"

case "$(printf "%s\n" "$(readlink -f "$1")" | tr '[:upper:]' '[:lower:]')" in
    *.tgz | *.tar.gz) tar tzf "$1" ;;
    *.tar.bz2 | *.tbz2) tar tjf "$1" ;;
    *.tar.txz | *.txz) xz --list "$1" ;;
    *.tar) tar tf "$1" ;;
    *.zip | *.jar | *.war | *.ear | *.oxt) unzip -l "$1" ;;
    *.rar) unrar l "$1" ;;
    *.7z) 7z l "$1" ;;
    *.[1-8]) man "$1" | col -b ;;
    *.o) nm "$1" ;;
    *.torrent) transmission-show "$1" ;;
    *.iso) iso-info --no-header -l "$1" ;;
    *.odt | *.ods | *.odp | *.sxw) odt2txt "$1" ;;
    *.doc) catdoc "$1" ;;
    *.docx) docx2txt "$1" - ;;
    *.xls | *.xlsx)
        ssconvert --export-type=Gnumeric_stf:stf_csv "$1" "fd://1" | batorcat --language=csv
        ;;
    *.wav | *.mp3 | *.flac | *.m4a | *.wma | *.ape | *.ac3 | *.og[agx] | *.spx | *.opus | *.as[fx] | *.mka)
        exiftool "$1"
        ;;
    *.pdf)
        [ ! -f "${CACHE}.jpg" ] && pdftoppm -jpeg -f 1 -singlefile "$1" "$CACHE"
        image "${CACHE}.jpg" "$2" "$3" "$4" "$5"
        ;;
    *.avi | *.mp4 | *.wmv | *.dat | *.3gp | *.ogv | *.mkv | *.mpg | *.mpeg | *.vob | *.fl[icv] | *.m2v | *.mov | *.webm | *.ts | *.mts | *.m4v | *.r[am] | *.qt | *.divx)
        [ ! -f "${CACHE}.jpg" ] && ffmpegthumbnailer -i "$1" -o "${CACHE}.jpg" -s 0 -q 5
        image "${CACHE}.jpg" "$2" "$3" "$4" "$5"
        ;;
    *.bmp | *.jpg | *.jpeg | *.png | *.xpm | *.webp | *.gif | *.jfif)
        image "$1" "$2" "$3" "$4" "$5"
        ;;
    *.svg)
        [ ! -f "${CACHE}.jpg" ] && convert "$1" "${CACHE}.jpg"
        image "${CACHE}.jpg" "$2" "$3" "$4" "$5"
        ;;
    *)
        batorcat "$1"
        ;;
esac
exit 0
