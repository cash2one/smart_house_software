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
import jieba.analyse
import json
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
# ... DEPRECATED.....
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
    text = strip_tags(preprocess) 
    text = SnowNLP(text)
    summary_text = ''
    for line in text.summary(1):
        summary_text = summary_text + line + '\n'
    
    print summary_text
    return summary_text
#这个函数实现知乎抓取功能
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

#这个函数实现新闻播报功能
def Spider_news():
        url = 'http://news.qq.com'
        webpage = urllib2.urlopen(url)
        text = webpage.read()
        text = unicode(text, 'gbk').encode('utf-8')
        soup = BeautifulSoup(text)
        new_1 = soup.findAll(attrs={'bosszone' :"focusnews1"})
        new_2 = soup.findAll(attrs={'bosszone' : "top2news"})
        new_3 = soup.findAll(attrs={'bosszone': "top3news"})
        new_1_page = new_1[0].a['href']
        new_2_page = new_2[0].a['href']
        new_3_page = new_3[0].a['href']
        text_1 = strip_tags(unicode(str(new_1[0]), 'utf-8').encode('utf-8'))
        text_2 = strip_tags(str(new_2[0]))
        text_3 = strip_tags(str(new_3[0]))
        title_1 = '一：' + text_1 + '。'
        title_2 = '二：' + text_2 + '。' 
        title_3 = '三：' + text_3 + '。' 
        tag_1 = jieba.analyse.extract_tags(title_1, topK=10)
        tag_2 = jieba.analyse.extract_tags(title_2, topK=10)
        tag_3 = jieba.analyse.extract_tags(title_3, topK=10)
        for tag1 in tag_1:
            config.CONTEXT_LINK[tag1] = new_1_page
        for tag2 in tag_2:
            config.CONTEXT_LINK[tag2] = new_2_page
        for tag3 in tag_3:
            config.CONTEXT_LINK[tag3] = new_3_page
        
        header = '有以下热点新闻：'
        final_news = header + '一：' + text_1 + '。' + '二：' + text_2 + '。' + '三：' + text_3
        final_news = final_news.replace(' ','，')
        print final_news
        return final_news

#这个函数实现新闻播报中二次询问的效果    
def Spider_news_context(query):
        for key in config.CONTEXT_LINK:
             print   key +' '+(config.CONTEXT_LINK[key])
        keywords = jieba.analyse.extract_tags(query, topK=5)
        for keyword in keywords:
            print keyword
            if keyword in config.CONTEXT_LINK:
                link = config.CONTEXT_LINK[keyword]
                break
        if link == None:
            return "在刚才的新闻里貌似找不到相关的内容。"
        page = link
        webpage = urllib2.urlopen(page)
        text = webpage.read()
        text = unicode(text, 'gbk').encode('utf-8')
        soup = BeautifulSoup(text)
        detail = soup.findAll(attrs = {'style':"TEXT-INDENT: 2em"})
        read_detail = strip_tags(unicode(str(detail), 'utf-8'))
        print type(read_detail)
        result =  re.findall(u'[0-9]*[《|“|、|：|(| ]*[\u4e00-\u9fa5]+[》|”|、|：|）| ]*[0-9]*[\u4e00-\u9fa5]*[；|，|。|？|！]',read_detail)
        final_result = ''
        for i in range(10):
            final_result = final_result + result[i];
        return final_result.encode('utf-8')

    
#这个函数实现智能菜谱抓取功能    
def Spider_meal():
    f = file('test.json')
    source = f.read()
    result = json.loads(source)
    link = None
    for content in result['data']['result']:
        if re.findall(r'(.)+(meishij)+(.)+',content['url']) != None :
            link = content['url']
            break
    if link == None:
        return "我找不到呢"
    webpage = urllib2.urlopen(link)
    text = webpage.read()
    soup = BeautifulSoup(text)
    detail = soup.findAll('p')
    text = strip_tags(str(detail))
    text = text[1:-1]
    print unicode(text,'utf-8').encode('utf-8')
    return unicode(text,'utf-8').encode('utf-8')

def Spider_world_cup():
    url = 'http://2014.qq.com/schedule/'
    webpage = urllib2.urlopen(url)
    text = webpage.read()
    soup = BeautifulSoup(text)
    detail = soup.findAll('tr')
    l = []
    t = ''
    finalresult = []
    for tips in detail:
        if 'status before' in str(tips):
            l.append(strip_tags(str(tips)))
    for i in l:
        i =  i.split('\n')[3:-1]
        for item in i:
            t = t + item + '，'
    return unicode(t,'utf-8').encode('utf-8')





def main():
    sentence1 = '设计师应该学编程吗'
    sentence2 = '西班牙'
    #Spider_meal()
    print Spider_world_cup()
    Spider_news()
    print Spider_news_context("我想听关于副省长的新闻")
#     Spider_zhihu(sentence1)
#     Spider_what(sentence2)
    
    

if __name__ == '__main__':
    main()
