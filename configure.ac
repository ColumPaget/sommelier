
AC_INIT([main.c])


AC_PROG_CC 
AC_LANG_C 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_HEADER_STDC 
AC_SYS_LARGEFILE

AC_CHECK_LIB([c],[prctl], AC_DEFINE([HAVE_PRCTL]))


AC_ARG_ENABLE(ssl, [  --enable-ssl       use Secure Sockets Layer (default='yes')], cf_use_ssl=$enableval, cf_use_ssl=yes )
AC_ARG_WITH(libuseful, [  --with-libuseful		use system-wide libUseful (default=auto) ], cf_system_libuseful=$withval )

if test "$cf_system_libuseful" != "no"
then
	AC_CHECK_LIB([Useful-5],GetToken,[cf_have_libUseful5=yes],[cf_have_libUseful5=no])
	if test "$cf_have_libUseful5" = "yes"
	then
	echo "installed libUseful found."
	AC_CHECK_LIB([Useful-5],AppendVar,,[cf_have_libUseful5=no])
  
	if test "$cf_have_libUseful5" = "yes"
	then
	AC_CHECK_HEADERS([libUseful-5/libUseful.h])
	else echo "installed libUseful too old. Must be v5.25 or newer"
	fi
	fi
fi

if test "$cf_have_libUseful5" = "yes" 
then
  echo "installed libUseful-5 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  AC_DEFINE(HAVE_LIBUSEFUL5)
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-5],GetToken)

	dnl openssl isn't really needed, but if it exists and we're using a system-wide libUseful, then that will
	dnl likely be linked against it
	AC_CHECK_LIB(ssl,SSL_library_init,,)
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
else
	AC_CHECK_LIB(z,inflate,,)
	AC_CHECK_LIB(ssl,SSL_library_init,,)
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)

  AC_DEFINE(USE_LIBUSEFUL_BUNDLED)
  AC_SUBST(LIBUSEFUL_BUNDLED,"libUseful-bundled/libUseful.a")
  AC_CONFIG_SUBDIRS([libUseful-bundled])
fi



if test "$cf_use_ssl" = "yes"
then
AC_CHECK_HEADER([openssl/crypto.h])
AC_CHECK_HEADER([openssl/ssl.h])

AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init

dnl New versions of openssl seem to lack EVP_CIPHER_CTX_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
if test "$cf_have_libcrypto" != "yes"
then
  AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new,,)
  cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_new
fi


AC_CHECK_LIB(ssl,SSL_library_init,,)
cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init

dnl New versions of openssl seem to lack SSL_library_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
if test "$cf_have_libssl" != "yes"
then
  AC_CHECK_LIB(ssl,OPENSSL_init_ssl,,)
  cf_have_libssl=$ac_cv_lib_ssl_OPENSSL_init_ssl
fi


fi


if test "$cf_have_libssl" = "yes"
then
	echo ""
else
	echo "$cf_have_libssl"
	echo "WARNING: SSL/TLS NOT enabled. Likely sommelier will not be able to download many applications."
fi

dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)

