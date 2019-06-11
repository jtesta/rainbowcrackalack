#!/bin/bash

if [[ $# != 2 ]]; then
    echo "This tool creates bash scripts to generate NTLM 8-character rainbow tables."
    echo
    echo "To make a script that creates tables from part index 50 to 75:"
    echo
    echo "    $0 50 75 > generate_ntlm_8char_50-75.sh; chmod 0700 *.sh"
    exit 1
fi

start=$1
end=$2

echo "#!/bin/bash"
echo
for i in `seq $start $end`; do
    echo "./crackalack_gen ntlm ascii-32-95 8 8 0 422000 67108864 $i"
done
