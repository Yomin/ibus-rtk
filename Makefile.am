
SUBDIRS = src icons dicts
EXTRA_DIST = autogen.sh @PACKAGE_NAME@.spec.in
dist_doc_DATA = README.md

rpm: dist @PACKAGE_NAME@.spec
	rpmbuild -bb \
        --define "_sourcedir `pwd`" \
        --define "_builddir `pwd`" \
        --define "_specdir `pwd`" \
        --define "_rpmdir `pwd`" \
        --define "_srcrpmdir `pwd`" \
        @PACKAGE_NAME@.spec
