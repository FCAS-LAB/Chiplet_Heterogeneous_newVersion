#!/usr/bin/env python
import os
import sys
import re

FILTER_1 = "10(0 failed) times"
FILTER_2 = "perf data"

FUN_NAME = ['Elapsed time', 'power']

def filter(file_path, filters, out_list):
    """
    根据filters条目 搜索file_path文件里对应的性能数据（数字数据）
    file_path 文件路径
    filters 刷选条目
    out_list 输出结果，格式为[筛选条目 数据]
    """
    flag = False
    #base_name = os.path.basename(file_path)
    path_list = []
    for fun_name in filters:
        txt_content = []
        with open(file_path, 'r', encoding = 'UTF-8', errors = 'ignore') as fr:
            for txt_line in fr.readlines():
                txt_content.append(txt_line.rstrip('\n')) #去掉每行末尾的换行符
            for line in txt_content:
                ret = line.find(fun_name)
                if -1 == ret:
                    continue
                real_line = line
                out_list.append('%s \t %s\n' % (fun_name, real_line))

def write2xls(xls_name, xls_content):
    with open('%s.xls' % xls_name, 'a+') as fw:
        fw.writelines(xls_content)

def ergodic_file(root_path, out_list, postfix):
    """
    查找当前目录所有以postfix为后缀的文件
    root_path 搜索目录
    out_list 搜索结果储存列表
    postfix 文件后缀名
    """
    if os.path.isdir(root_path):
        for file in os.listdir(root_path):
            sub_path = os.path.join(root_path, file)
            if os.path.isfile(sub_path):
                if sub_path.endswith(postfix):
                    out_list.append(sub_path)
    else:
        return 0
    if not out_list:
        print("No %s file have found." % (postfix))
        return 0
    return 1


def ger_info_by_type(flist, fun_list):
    '''
    根据提供的信息查找文件对应内容，并写入xls
    flist 文件列表
    fun_list 函数名列表
    '''
    for file in flist:
        out = []
        filter(file, fun_list, out)
        write2xls('performance', out)


def main():
    xls_file_list = []
    txt_file_list = []
    print("---find start, please wait---")
    #删除当前目录下的*.xls文件
    ergodic_file(sys.path[0], xls_file_list, ".xls")
    for xls_file in xls_file_list:
        os.remove(xls_file)
    #开始刷选
    for s in os.listdir(sys.path[0]):
        newDir=os.path.join(sys.path[0],s)
        ret = ergodic_file(newDir, txt_file_list, ".log")
        if ret:
            ger_info_by_type(txt_file_list, FUN_NAME)
    print("search done") 


# main begin
if __name__=='__main__':
    main()
    os.system("pause")
