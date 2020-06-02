#!/bin/bash

USAGE="Usage: ./create_infiles.sh <diseasesFile> <countriesFile> <input_dir> <numFilesPerDir> <numRecordsPerFile>"

if [ $# -ne 5 ];
then
    echo ${USAGE} ;
    exit -1
fi

diseasesFile=$1
countriesFile=$2
input_dir=$3
numFilesPerDir=$4
numRecordsPerFile=$5

if [ ! -d $input_dir ]
then
    mkdir $input_dir
fi 


while IFS= read -r country
do
    mkdir $input_dir/$country

    for (( i=0; i<$numFilesPerDir; i++))
    do
        day=$((1 + RANDOM % 30))
        month=$((1 + RANDOM % 12))
        year=$((2000 + RANDOM % 19))

        while [ -a $input_dir/$country/$day-$month-$year.txt ]
        do
            day=$((1 + RANDOM % 30))
            month=$((1 + RANDOM % 12))
            year=$((2000 + RANDOM % 19))

        done

        touch $input_dir/$country/$day-$month-$year.txt

        for (( j=0; j<$numRecordsPerFile; j++ ))
        do
            id=$((RANDOM % 1000))
            status="ENTER"
            
            ran=$((RANDOM % 20))
            if [ $ran -lt 5 ];
            then
                status="EXIT"
            fi

            # if the name is random the programs whas a very small chanse to validate someone for an exit date (exit date name == entry date name)

            fname=Jhon 
            lname=Davidson

            # fname=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13 ; echo '')
            # lname=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 13 ; echo '')

            disease=$(shuf -n 1 $diseasesFile)
            age=$((1 + RANDOM % 120))
            
            line=$id" "$status" "$fname" "$lname" "$disease" "$age
            echo $line >> $input_dir/$country/$day-$month-$year.txt
        done 
    done

done < $countriesFile 
