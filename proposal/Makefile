%.html : %.rst
	@rst2html.py --stylesheet=style.css,pygments.css $< $@

p0468.html: p0468.rst style.css pygments.css

all: p0468.html

clean:
	@rm *.html
