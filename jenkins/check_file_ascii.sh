#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

ERRFILE=check_ascii.sh.err
ERRFILETEMP=$ERRFILE.0

# display a helpful message for someone reading the log
echo "Check ascii > Checking $1"

if [ ! -e $1 ]; then # the file wasn't present; not necessarily an error
    echo "WARNING: file not found: $1"
    exit 0 # don't report an error but don't run the rest of this file
fi

# grep for non-ascii - also exclude unprintable control characters at the end of the range
# specifically include x09 (tab) as it is used in pal sources which are not excluded
# from this check
LC_CTYPE=C grep -nP '[^\x09-\x7E]' $1 > $ERRFILETEMP
if [ $? -eq 0 ]; then # grep found matches ($?==0), so we found non-ascii in the file
    echo "ERROR: non-ascii characters were introduced in $1" >> $ERRFILE

    # Display a hexdump sample of the lines with non-ascii characters in them
    # Don't pollute the log with every single matching line, first 10 lines should be enough.
    echo "Displaying first 10 lines of text where non-ascii characters were found:" >> $ERRFILE
    LC_CTYPE=C grep -nP '[^\x09-\x7E]' $1 | xxd -g 1 > $ERRFILETEMP
    head -n 10 $ERRFILETEMP >> $ERRFILE

    # To help the user, display how many lines of text actually contained non-ascii characters.
    LINECOUNT=`python -c "file=open('$ERRFILETEMP', 'r'); print len(file.readlines())"`
    echo "Total lines containing non-ascii: $LINECOUNT" >> $ERRFILE
    echo "--------------" >> $ERRFILE # same length as '--- ERRORS ---'
fi

rm -f $ERRFILETEMP
