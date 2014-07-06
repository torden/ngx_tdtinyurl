# Install Redis client library for python
```
git clone https://github.com/andymccurdy/redis-py
cd redis-py/
python setup.py build
python setup.py install
```
    
# Install KyotoCabinet client library for python
```
wget http://fallabs.com/kyotocabinet/pythonlegacypkg/kyotocabinet-python-legacy-1.18.tar.gz
tar xvzf kyotocabinet-python-legacy-1.18.tar.gz
cd kyotocabinet-python-legacy-1.18

vi setup.py

Edit

 27     include_dirs = ['/usr/local/include']

to 

 27     include_dirs = ['/usr/local/kyotocabinet/include']


 43     library_dirs = ['/usr/local/lib']

to

 43     library_dirs = ['/usr/local/kyotocabinet/lib']

python setup.py build
python setup.py install
```
 
