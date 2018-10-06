
AC_INIT([main.c])


AC_PROG_CC 
AC_LANG_C 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_HEADER_STDC 
AC_SYS_LARGEFILE


AC_ARG_ENABLE(ssl, [  --enable-ssl       use Secure Sockets Layer], cf_use_ssl=$enableval, cf_use_ssl=yes )

echo "use: $cf_use_ssl"
if test "$cf_use_ssl" = "yes"
then
AC_CHECK_HEADER([openssl/crypto.h])
AC_CHECK_HEADER([openssl/ssl.h])

AC_CHECK_LIB(ssl,SSL_library_init,,)
cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init

AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init
fi


STATIC_LIBS="libUseful-3/libUseful.a"
AC_SUBST(STATIC_LIBS)
AC_CONFIG_SUBDIRS([libUseful-3])


if test "$cf_have_libssl" = "yes"
then
				echo ""
else
				echo "$cf_have_libssl"
				echo "WARNING: SSL/TLS NOT enabled. Likely sommelier will not be able to download many applications."
fi

dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)

