#!/bin/bash
if [ $# -eq 3 ] 
    then
        INPUTDIR=${1}
        OUTPUTDIR=${2}
        MAXTHREADS=${3}

        for input in $INPUTDIR/*.txt
        do
            for i in $(seq 1 $MAXTHREADS)
            do
                name=${input##*/}
                name=${name%.*}
                echo InputFile=$name NumThreads=$i
                ./tecnicofs $input $OUTPUTDIR/${name}-${i}.txt $i | grep "TecnicoFS"
            done
        done
    else
        echo "erro:argumentos invalidos"
fi