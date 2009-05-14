#GT_DOMAIN:=gPlanarity
SRC=../*.c
ifeq ($(origin WANT_LINGUAS), undefined)
PO=$(wildcard *.po)
else
PO=$(addsuffix .po,$(WANT_LINGUAS))
endif
GMO=$(PO:.po=.gmo)

HEADER=--msgid-bugs-address="Monty <monty@xiph.org>"

all: $(GT_DOMAIN).pot $(GMO)

$(GT_DOMAIN).pot: $(SRC)
	xgettext --strict $(HEADER) -d $(GT_DOMAIN) -o /dev/stdout \
		-L C  --keyword=_ $(SRC) | sed -f header.sed >$@

%.gmo: %.po $(GT_DOMAIN).pot
	msgmerge -o$<n $< $(GT_DOMAIN).pot
	mv $<n $<
	msgfmt --statistics --check --strict -o $@ $<

clean:
	rm -f $(GMO) *.pon

install:
	for F in $(GMO); do \
		export LL=`echo $$F|sed 's/\.gmo//'` ;\
		$(INSTALL) -m 0644 -D "$$F" "$(GT_DIR)/$${LL}/LC_MESSAGES/$(GT_DOMAIN).mo" ;\
	done

