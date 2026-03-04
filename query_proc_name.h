#pragma once

#include <Windows.h>
#include <iostream>
#include <tchar.h>
// 标准wcsrchr实现（宽字符版反向字符查找）
wchar_t *mywcsrchr(const wchar_t *str, wchar_t wc)
{
    // ? 边界检查：传入的字符串指针为NULL时直接返回NULL（符合C标准）
    if (str == NULL)
    {
        return NULL;
    }

    const wchar_t *last_match = NULL; // 存储最后一次匹配的位置（反向查找即第一个匹配）
    const wchar_t *p = str;           // 遍历指针

    // ? 遍历整个宽字符字符串（直到终止符L'\0'）
    for (; *p != L'\0'; ++p)
    {
        // 找到匹配的宽字符，记录当前位置
        if (*p == wc)
        {
            last_match = p;
        }
    }

    // ? 特殊处理：如果查找的是终止符L'\0'，直接返回指向末尾的指针
    if (wc == L'\0')
    {
        last_match = p; // p此时指向字符串末尾的L'\0'
    }

    // ? 返回匹配结果（未找到则为NULL），移除const（符合标准的隐式转换）
    return (wchar_t *)last_match;
}


int mywcsicmp(const wchar_t *str1, const wchar_t *str2)
{
    for (; *str1 != L'\0' && *str2 != L'\0'; str1++, str2++)
    {
        // 核心：转换为小写后比较
        wchar_t diff = towlower((wint_t)*str1) - towlower((wint_t)*str2);
        if (diff != 0)
        {
            return (int)diff;
        }
    }

    // 处理长度不一致的情况
    return (int)(towlower((wint_t)*str1) - towlower((wint_t)*str2));
}
