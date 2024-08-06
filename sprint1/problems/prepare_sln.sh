#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

#echo "$SCRIPT_DIR"
echo "Enter Problem folder name: ..."

read TASK_FOLDER
TASK_FOLDER_PATH="$SCRIPT_DIR/$TASK_FOLDER"
echo " -> Full path to problem is: " "$TASK_FOLDER_PATH"

cp -rf $TASK_FOLDER/precode/* $TASK_FOLDER/solution/

mkdir $TASK_FOLDER/solution/build
cd $TASK_FOLDER/solution/build

conan install --build missing ..
cmake ..