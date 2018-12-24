from gensim.models import Word2Vec, KeyedVectors
from collections import Counter
from tabulate import tabulate
from sklearn.manifold import TSNE
import matplotlib.pyplot as plt
import seaborn as sns
import re
import logging
#CBOW 50 HS

def build_m(fr_srt,fw_str,model_str,logw,logs,num_epochs,plt=0):
	fh=open(fr_srt,'r'); fhw=open(fw_str,'w+')
	vocab=[]
	for line in fh.readlines():
		vocab+=re.sub('[^A-Za-zА-Яа-яІіЫыЄєїЭэъ0-9 ]','',line).lower().split(' ')
		fhw.write(re.sub('[^A-Za-zА-Яа-яІіЫыЄєїЭэъ0-9 ]','',line).lower()+'\n')
	c=Counter(vocab)
	lst_w_f=list(zip(*c.most_common()))
	logging.basicConfig(format="%(levelname)s - %(asctime)s: %(message)s", datefmt= '%H:%M:%S', level=logging.INFO)
	w2v_model = Word2Vec(
		size=len(lst_w_f[0]),#Dimensionality of the word vectors;
		window=2,#Maximum distance between the current and predicted word within a sentence
		min_count=0,#Ignores all words with total frequency lower than this
		workers=3,#Use these many worker threads to train the model (=faster training with multicore machines)
		sg=0,#Training algorithm: 1 for skip-gram; otherwise CBOW
		)
	fhw.flush(); fhw.seek(0,0)
	w2v_model.build_vocab(corpus_file=fw_str, progress_per=logw)
	w2v_model.train(corpus_file=fw_str, total_words=len(lst_w_f[0]), epochs=num_epochs, report_delay=logs)
	w2v_model.wv.save_word2vec_format(model_str)
	fh.close(); fhw.close()
	if plt:
		vectors=[]
		for wrd in w2v_model.wv.vocab:
			vectors.append(w2v_model[wrd])
		xy=TSNE(n_components=2, perplexity=15, init='pca', random_state=0,).fit_transform(vectors)
		print('{} {} {}'.format(len(lst_w_f[0]), len(vectors), type(vectors)))
		#print('{} {}'.format( len(vectors), type(vectors)))
		sns.jointplot(xy[:,0], xy[:,1], height=10)
		
	
tst_words_lst=[['наука','вивчати'],['географія','земля'],['ератосфен','птолемей'],['економічний','соціальний'],['транспорт','транспортний']]
siml_words_lst=['наука','дослідження','розвиток','структура','географічний']

table_tst=[]; lsttmp=[]
build_m('txts_r/q50.txt','txts_w/q50w.txt','models/modelq50',30,2,30,1);
w2v_model = Word2Vec(); w2v_model.wv=KeyedVectors.load_word2vec_format('models/modelq50')
for word1, word2 in tst_words_lst:
	lsttmp.append(w2v_model.wv.similarity(word1,word2))
lsttmp.insert(0,'model1'); table_tst.append(lsttmp); lsttmp=[]
for word in siml_words_lst:
	print('{}:\n'.format(word))
	print(w2v_model.wv.most_similar(positive=[word]))
plt.show()

build_m('txts_r/q100.txt','txts_w/q100w.txt','models/modelq100',30,2,30,1)
w2v_model = Word2Vec(); w2v_model.wv=KeyedVectors.load_word2vec_format('models/modelq100')
for word1, word2 in tst_words_lst:
	lsttmp.append(w2v_model.wv.similarity(word1,word2))
lsttmp.insert(0,'model2'); table_tst.append(lsttmp); lsttmp=[]
for word in siml_words_lst:
	print('{}:\n'.format(word))
	print(w2v_model.wv.most_similar(positive=[word]))
plt.show()

build_m('txts_r/q250.txt','txts_w/q250w.txt','models/modelq250',100,4,30)
w2v_model = Word2Vec(); w2v_model.wv=KeyedVectors.load_word2vec_format('models/modelq250')
for word1, word2 in tst_words_lst:
	lsttmp.append(w2v_model.wv.similarity(word1,word2))
lsttmp.insert(0,'model3'); table_tst.append(lsttmp); lsttmp=[]
for word in siml_words_lst:
	print('{}:\n'.format(word))
	print(w2v_model.wv.most_similar(positive=[word]))
srt_tmp=input('pak...')

build_m('txts_r/q500.txt','txts_w/q500w.txt','models/modelq500',100,5,30)
w2v_model = Word2Vec(); w2v_model.wv=KeyedVectors.load_word2vec_format('models/modelq500')
for word1, word2 in tst_words_lst:
	lsttmp.append(w2v_model.wv.similarity(word1,word2))
lsttmp.insert(0,'model4'); table_tst.append(lsttmp); lsttmp=[]
for word in siml_words_lst:
	print('{}:\n'.format(word))
	print(w2v_model.wv.most_similar(positive=[word]))
srt_tmp=input('pak...')

build_m('txts_r/q1000.txt','txts_w/q1000w.txt','models/modelq1000',250,7,30)
w2v_model = Word2Vec(); w2v_model.wv=KeyedVectors.load_word2vec_format('models/modelq1000')
for word1, word2 in tst_words_lst:
	lsttmp.append(w2v_model.wv.similarity(word1,word2))
lsttmp.insert(0,'model5'); table_tst.append(lsttmp); lsttmp=[]
for word in siml_words_lst:
	print('{}:\n'.format(word))
	print(w2v_model.wv.most_similar(positive=[word]))

table_tst=list(zip(*table_tst)); tst_words_lst.insert(0,['',''])
for indx in range(len(table_tst)):
	table_tst[indx]=list(table_tst[indx])
	table_tst[indx].insert(0,tst_words_lst[indx][0]+' '+tst_words_lst[indx][1])
print(tabulate(table_tst,tablefmt='orgtbl'))






