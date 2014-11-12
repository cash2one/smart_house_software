#!/usr/bin/env python
# -*- coding: utf-8 -*- 
'''
Created on 2014年6月13日

@author: wilsoncao
'''

import urllib2
from BeautifulSoup import BeautifulSoup
from HTMLParser import HTMLParser
from snownlp import SnowNLP
import config
import re
# import chardet

class MLStripper(HTMLParser):
    def __init__(self):
        self.reset()
        self.fed = []
    def handle_data(self, d):
        self.fed.append(d)
    def get_data(self):
        return ''.join(self.fed)
    
def strip_tags(html):
    s = MLStripper()
    s.feed(html)
    return s.get_data()



# This function is used for capture the content from the baidu jingyan website.
# The content will be used as a input of the responder.
def Spider_how(sentence):
    sentence = urllib2.quote(sentence)
    url  = 'http://jingyan.baidu.com/search?word='
    finalUrl = url + sentence
    webpage = urllib2.urlopen(finalUrl)
    text = webpage.read()
    soup = BeautifulSoup(text)
    website = soup.findAll(log = "type:1000,pos:search_result,num:1")[0]
    postfix = website['href']
    result = 'http://jingyan.baidu.com'
    finalresult = result + postfix
    resultpage = urllib2.urlopen(finalresult)
    text = resultpage.read()
    result_soup = BeautifulSoup(text)
    readable_text = result_soup.findAll('p')
    readable_text = readable_text[1:]
    preprocess = ''
    for line in readable_text:
        for sub_line in str(line).split('。'):
            preprocess = preprocess + sub_line + '\n'
    text = strip_tags(preprocess)
    print text
    text = SnowNLP(text)
    summary_text = ''
    for line in text.summary(3):
        summary_text = summary_text + line+'\n'
    print summary_text
    return summary_text

def Spider_what(sentence):
    sentence = urllib2.quote(sentence)
    url = 'http://zh.wikipedia.org/wiki/'
    finalUrl = url + sentence
    webpage = urllib2.urlopen(finalUrl)
    text = webpage.read()
    soup = BeautifulSoup(text)
    readable_text = soup.findAll('p')
    preprocess = ''
    for line in readable_text:
        for sub_line in str(line).split('。'):
            preprocess = preprocess + sub_line + '\n'
    #readable_text = str(readable_text)[1:-1]
    #preprocess = unicode(preprocess,'utf-8').encode('utf-8')
    text = strip_tags(preprocess) 
    text = SnowNLP(text)
    summary_text = ''
    for line in text.summary(1):
        summary_text = summary_text + line + '\n'
    
    print summary_text
    return summary_text

def Spider_zhihu(sentence):
    sentence = urllib2.quote(sentence)
    url  = 'http://www.zhihu.com/search?q='
    finalUrl = url + sentence + '&type=question'
    webpage = urllib2.urlopen(finalUrl)
    text = webpage.read()
    soup = BeautifulSoup(text)
    website = soup.findAll(attrs = {'class' :"question_link"})[0]
    postfix = website['href']
    result = 'http://www.zhihu.com'
    finalresult = result + postfix
    resultpage = urllib2.urlopen(finalresult)
    text = resultpage.read()
    result_soup = BeautifulSoup(text)
    readable_text = result_soup.findAll(attrs = {'data-action' :'/answer/content'})[0]
    #readable_text = readable_text[1:]
    print readable_text
    preprocess = ''
    for line in readable_text:
        for sub_line in str(line).split('。'):
            preprocess = preprocess + sub_line + '\n'
    text = strip_tags(preprocess)
    print text
    text = SnowNLP(text)
    summary_text = ''
    for line in text.summary(3):
        summary_text = summary_text + line+'。'
    print summary_text
    return summary_text


def Spider_news():
        url = 'http://news.qq.com'
        webpage = urllib2.urlopen(url)
        text = webpage.read()
        text = unicode(text, 'gbk').encode('utf-8')
        soup = BeautifulSoup(text)
        new_1 = soup.findAll(attrs={'bosszone' :"focusnews1"})
        new_2 = soup.findAll(attrs={'bosszone' : "top2news"})
        new_3 = soup.findAll(attrs={'bosszone': "top3news"})
        text_1 = strip_tags(unicode(str(new_1[0]), 'utf-8').encode('utf-8'))
        text_2 = strip_tags(str(new_2[0]))
        text_3 = strip_tags(str(new_3[0]))
        header = '有以下热点新闻：'
        final_news = header + '一：' + text_1 + '。' + '二：' + text_2 + '。' + '三：' + text_3
        print final_news
        config.CONTEXT = 1
        return final_news
    
def Spider_news_context(s):
        url = 'http://news.qq.com'
        webpage = urllib2.urlopen(url)
        text = webpage.read()
        text = unicode(text, 'gbk').encode('utf-8')
        soup = BeautifulSoup(text)
        if s == 0:
            news = soup.findAll(attrs={'bosszone' :"focusnews1"})
        elif s ==1 :
            news = soup.findAll(attrs={'bosszone' : "top2news"})
        elif s == 2:
            news = soup.findAll(attrs={'bosszone': "top3news"})
        page = news[0].a['href']
        webpage = urllib2.urlopen(page)
        text = webpage.read()
        text = unicode(text, 'gbk').encode('utf-8')
        soup = BeautifulSoup(text)
        detail = soup.findAll(attrs = {'style':"TEXT-INDENT: 2em"})
        #print detail
        read_detail = strip_tags(unicode(str(detail), 'utf-8'))
        #read_detail = re.match(u"[\u4e00-\u9fa5]+",read_detail)
#         sum_detail = SnowNLP(read_detail)
#         sum_detail = sum_detail.summary(3)
#         xx=ur"([/u4e00-/u9fa5]+)"  
#         pattern = re.compile(xx)  
#         print pattern.findall(str(detail).decode('utf8'))
        #print detail
        result =  re.findall(u'[0-9]*[《]*[\u4e00-\u9fa5]+[》]*[0-9]*[\u4e00-\u9fa5]*[，|。|？|！]',str(detail).decode('utf-8'))
        finalresult = ''
        for i in range(10):
            finalresult = finalresult + result
#         print chardet.detect(str(detail))['encoding']
       # print unicode(str(detail),'gbk')
        return finalresult

    
    


def main():
    sentence1 = '设计师应该学编程吗'
    sentence2 = '西班牙'
    Spider_news_context(2)
#     Spider_zhihu(sentence1)
#     Spider_what(sentence2)
    
    

if __name__ == '__main__':
    main()
