
# Downloads $1 (URL) to $2 (target destination) using wget or curl,
# depending on OS.
# E.g. generic_download_file www.url.com/my.zip loc/my.zip
function generic_download_file {
    if [[ $# -ne 2 ]]; then
        echo "$0: bad args to generic_download_file!"
        exit 1
    fi

    if [[ -f "$2" ]]; then
        echo "File $2 exists, skip download..."
        return
    fi

    local download_failed=false
    if type curl &> /dev/null; then
        if ! curl -L "$1" -o "$2"; then
            download_failed=true
        fi
    elif type wget &> /dev/null; then
        if ! wget -c "$1" -O "$2"; then
            download_failed=true
        fi
    else
        echo "Cannot find download tool. Please install curl or wget."
        exit 1
    fi

    if $download_failed; then
        echo "Failed to download $1"
        rm -f "$2"
        exit 1
    fi
}
