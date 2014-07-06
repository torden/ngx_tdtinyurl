# Install Ruby

```bash
wget http://cache.ruby-lang.org/pub/ruby/2.1/ruby-2.1.2.tar.gz
tar xvzf ruby-2.1.2.tar.gz
cd ruby-2.1.2
./configure --prefix=/usr/local/ruby --with-static-linked-ext --enable-shared --enable-pthread
gmake
gmake install

```

# Install Kyotocabinet for Ruby

```bash
wget http://fallabs.com/kyotocabinet/rubypkg/kyotocabinet-ruby-1.32.tar.gz
tar xvzf kyotocabinet-ruby-1.32.tar.gz
cd kyotocabinet-ruby-1.32
vi extconf.rb

Edit 14,15 lines of extconf.rb

 14 kccflags = "-I/usr/local/kyotocabinet/include" if(kccflags.length < 1)
 15 kcldflags = "-L/usr/local/kyotocabinet/lib" if(kcldflags.length < 1)

to

 14 kccflags = "-I/usr/local/kyotocabinet/include" if(kccflags.length < 1)
 15 kcldflags = "-L/usr/local/kyotocabinet/lib" if(kcldflags.length < 1)


/usr/local/ruby/bin/ruby extconf.rb

Edit 82 line of extconf.rb

 82 INCFLAGS = -I. -I$(arch_hdrdir) -I$(hdrdir)/ruby/backward -I$(hdrdir) -I$(srcdir)
 
to

 82 INCFLAGS = -I. -I/usr/local/kyotocabinet/include -I$(arch_hdrdir) -I$(hdrdir)/ruby/backward -I$(hdrdir) -I$(srcdir)

gmake
gmake install

```

# Install Redis-rb

```bash
/usr/local/ruby/bin/gem install redis
```
