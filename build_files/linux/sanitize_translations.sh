#!/bin/bash
echo `pwd`
sed -i '/\-I\/usr\/include/d' CMakeFiles/lang_*
sed -i '/\-I\/usr\/include/d' CMakeFiles/source_lst_file
