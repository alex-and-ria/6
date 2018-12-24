from collections import Counter
from time import time
from gensim.models import Word2Vec
from tabulate import tabulate
import matplotlib.pyplot as plt
import re
import logging
import math
#CBOW 50 HS

cnt=0; cntw=0; cntl=0; cntuqw=0; M_dims=5; vocab=[]

fh=open("txts_r/q_.txt",'r')
fhw=open("txts_w/q_w.txt",'w+')
lines=fh.readlines()
for line in lines:#list of separate words
	vocab+=re.sub('[^A-Za-zА-Яа-яІіЫыЄєїЭэъ0-9 ]','',lines[cntl]).lower().split(' ')
	#rint('![%s]' % ','.join(map(str,lines[cntl].translate(None,'\'=.-\n').split(' '))))	print('\n[%s]' % ','.join(map(str,vocab)))
	#print(lines[cntl].translate(str.maketrans('','','\'=.-\n')).lower())
	fhw.write(re.sub('[^A-Za-zА-Яа-яІіЫыЄєїЭэъ0-9 ]','',lines[cntl]).lower()+'\n')
	cntl+=1

for word in vocab:
	vocab[cntw]=[vocab[cntw],1]
	#print('word={}, num={}'.format(vocab[cntw][0],vocab[cntw][1]))
	cntw+=1

cntuqw=cntw; cnt1=0; to_hist=[]
for word in vocab:
	#print('\nword={}, cnt1={}, cnt={}'.format(word[0],cnt1,cnt))
	to_hist.append(str(cnt1))
	for cnt in range(cntuqw-1):
		if (cnt1+cnt+1)<cntuqw:
			if word[0]==vocab[cnt1+cnt+1][0]:
				#print('word={}, cnt1={}, cnt={}, cntuqw={}'.format(word[0],cnt1,cnt,cntuqw))
				word[1]+=1
				del vocab[cnt1+cnt+1]
				cnt-=1
				cntuqw-=1
				to_hist.append(str(cnt1))
	#srt_tmp=input('pak')
	cnt1+=1

c=Counter(to_hist);
plt.bar(*zip(*c.most_common()))
plt.show()

print('cntw={}, cntuqw={}'.format(cntw,cntuqw))

#for word in vocab:
#	print('{}, {}'.format(word[0],word[1]))

logging.basicConfig(format="%(levelname)s - %(asctime)s: %(message)s", datefmt= '%H:%M:%S', level=logging.INFO)
w2v_model = Word2Vec(
	size=cntuqw,#Dimensionality of the word vectors;
	window=2,#Maximum distance between the current and predicted word within a sentence
	min_count=0,#Ignores all words with total frequency lower than this
	workers=1,#Use these many worker threads to train the model (=faster training with multicore machines)
	sg=0,#Training algorithm: 1 for skip-gram; otherwise CBOW
	)
fhw.flush(); fhw.seek(0,0)
w2v_model.build_vocab(corpus_file='txts_w/q_w.txt', progress_per=50)
w2v_model.train(corpus_file='txts_w/q_w.txt', total_words=cntuqw, epochs=30, report_delay=1)

def E_d(vects0, vects1):
	return math.sqrt(sum([(vects0[indx]-vects1[indx])*(vects0[indx]-vects1[indx]) for indx in range(len(vects0))]))
	
def E_d_n(vects0, vects1):
	lst0=[(x+0.0)/max(vects0) for x in vects0]
	lst1=[(x+0.0)/max(vects1) for x in vects1]
	return E_d(lst0,lst1)
	
def cos_d(vects0,vects1):
	scal_prod=sum([vects0[indx]*vects1[indx] for indx in range(len(vects0))])+0.0
	a0_len=sum([x*x for x in vects0])
	a1_len=sum([x*x for x in vects1])
	#print('{} {} {}'.format(scal_prod,a0_len,a1_len))
	return 1-scal_prod/(math.sqrt(a0_len*a1_len))
	
def cos_d_n(vects0,vects1):
	lst0=[(x+0.0)/max(vects0) for x in vects0]
	lst1=[(x+0.0)/max(vects1) for x in vects1]
	return cos_d(lst0,lst1)
	
m_lst=[]; vects_lst=[];
keys_lst=list(w2v_model.wv.vocab.keys())[:5]
vects_lst.extend([w2v_model[key] for key in keys_lst])
for vect0 in vects_lst:
	tmp_lst=[]
	for vect1 in vects_lst:
		tmp_lst.append([round(E_d(vect0,vect1),2),round(cos_d(vect0,vect1),2),round(E_d_n(vect0,vect1),2),round(cos_d_n(vect0,vect1),2)])
	m_lst.append(tmp_lst)
	
print(tabulate(m_lst,tablefmt='orgtbl'))


	
fh.close()
fhw.close()
