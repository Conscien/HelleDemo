#!/bin/bash

# 格式化
dirs_arrays="ffmpegEncoder serviceModuleDllSo zmqdemo template"
files_arrays="*.h *.hpp *.c *.cpp *.cc"
for i in $dirs_arrays; do
  for j in $files_arrays; do
    clang-format -i `find $i -type f -name $j`
  done
done

# 统计代码行数
cloc --git $(git branch --show-current)
