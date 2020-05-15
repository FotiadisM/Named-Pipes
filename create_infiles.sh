#!/bin/bash

USAGE="Usage: ./create_infiles.sh <diseasesFile> <countriesFile> <input_dir> <numFilesPerDir> <numRecordsPerFile>"

if [ $# -ne 5 ];
then
    echo ${USAGE} ;
    exit 1
fi

diseasesFile=$1
countriesFile=$2
input_dir=$3
numFilesPerDir=$4
numRecordsPerFile=$5

for (( i=0; i<$numFilesPerDir; i++ ))
do
    newDir=$(shuf -n 1 $countriesFile)

    while [ -d "$input_dir/$newDir" ]
    do
        newDir=$(shuf -n 1 $countriesFile)
    done

    mkdir $input_dir/$newDir
done
