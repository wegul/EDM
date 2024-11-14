cd ./

mkdir -p "out/testdir"
mkdir -p "results"

testdir="testdir"
programPath="./bin/"
declare -a programList=("edm" "ird" "pfabric" "pfc" "dctcp" "cxl" "fastpass")

for pname in "${programList[@]}"; do
    program="${programPath}${pname}"
    echo "${program}"
    for file in ${testdir}/*.csv; do
        if [[ -f ${file} ]]; then
            echo "screen -dmS ${pname}-${file##*\/} bash -c \"$program -f $file >${file}_${pname}.log\" "
            screen -dmS ${pname}-${file##*\/} bash -c "$program -f $file >${file}_${pname}.log"
        fi
    done
done

