#!/bin/bash
FRED_HASH_VER=0.1
DNS_CACHE_VER=0.1
GSSCRED_CACHE_VER=0.1
HEIMDAL_VER=1.5.3.107
OPENSSL_VER=1.0.1e
FBOPENSSL_VER=0.0.4.106
CARES_VER=1.7.5
#LIBEVENT_VER=2.0.21-stable
LIBEVENT_VER=1.4.14b-stable
#CURL_VER=7.33.0.107
CURL_VER=7.24.0.107
#CURL_VER=7.34.0
#CURL_VER=bad
HIREDIS_VER=0.11.0

INSTALL_PATH=/usr/curl-loader
CL_INCLUDE_PATH=$(pwd)/inc
CL_LIB_PATH=$(pwd)/lib
CL_OBJ_PATH=$(pwd)/obj

FRED_HASH_INSTALL_PATH=$INSTALL_PATH/fred_hash
DNS_CACHE_INSTALL_PATH=$INSTALL_PATH/dns_cache
GSSCRED_CACHE_INSTALL_PATH=$INSTALL_PATH/gsscred_cache
HEIMDAL_INSTALL_PATH=$INSTALL_PATH/heimdal
OPENSSL_INSTALL_PATH=$INSTALL_PATH/openssl
FBOPENSSL_INSTALL_PATH=$INSTALL_PATH/fbopenssl
CARES_INSTALL_PATH=$INSTALL_PATH/c-ares
LIBEVENT_INSTALL_PATH=$INSTALL_PATH/libevent
CURL_INSTALL_PATH=$INSTALL_PATH/curl
HIREDIS_INSTALL_PATH=$INSTALL_PATH/hiredis

FRED_HASH_LIB_PATH=$FRED_HASH_INSTALL_PATH/lib
FRED_HASH_INCLUDE_PATH=$FRED_HASH_INSTALL_PATH/include
DNS_CACHE_LIB_PATH=$DNS_CACHE_INSTALL_PATH/lib
DNS_CACHE_INCLUDE_PATH=$DNS_CACHE_INSTALL_PATH/include
GSSCRED_CACHE_LIB_PATH=$GSSCRED_CACHE_INSTALL_PATH/lib
GSSCRED_CACHE_INCLUDE_PATH=$GSSCRED_CACHE_INSTALL_PATH/include
HEIMDAL_LIB_PATH=$HEIMDAL_INSTALL_PATH/lib
HEIMDAL_INCLUDE_PATH=$HEIMDAL_INSTALL_PATH/include
OPENSSL_LIB_PATH=$OPENSSL_INSTALL_PATH/lib
OPENSSL_INCLUDE_PATH=$OPENSSL_INSTALL_PATH/include
FBOPENSSL_LIB_PATH=$FBOPENSSL_INSTALL_PATH/lib
FBOPENSSL_INCLUDE_PATH=$FBOPENSSL_INSTALL_PATH/include
CARES_LIB_PATH=$CARES_INSTALL_PATH/lib
CARES_INCLUDE_PATH=$CARES_INSTALL_PATH/include
LIBEVENT_LIB_PATH=$LIBEVENT_INSTALL_PATH/lib
LIBEVENT_INCLUDE_PATH=$LIBEVENT_INSTALL_PATH/include
CURL_LIB_PATH=$CURL_INSTALL_PATH/lib
CURL_INCLUDE_PATH=$CURL_INSTALL_PATH/include
HIREDIS_LIB_PATH=$HIREDIS_INSTALL_PATH/lib
HIREDIS_INCLUDE_PATH=$HIREDIS_INSTALL_PATH/include

#DEBUG_FLAGS="-g"
OPT_FLAGS_WITH_DEBUG="-O3 -ffast-math -finline-functions -funroll-all-loops -finline-limit=1000 -mmmx -msse -foptimize-sibling-calls -g"
OPT_FLAGS_WITHOUT_DEBUG="-O3 -ffast-math -finline-functions -funroll-all-loops -finline-limit=1000 -mmmx -msse -foptimize-sibling-calls -fomit-frame-pointer"

copy_libs_and_incs(){
    for lib_and_inc_path in OPENSSL HEIMDAL FBOPENSSL CARES LIBEVENT FRED_HASH DNS_CACHE GSSCRED_CACHE HIREDIS
    do
        eval cp -rf \$"$lib_and_inc_path"_INCLUDE_PATH/* $CL_INCLUDE_PATH
        eval cp -f \$"$lib_and_inc_path"_LIB_PATH/lib*a $CL_LIB_PATH
    done
}

config_ld_path(){    #$1: lib path
    if [ `grep $1 /etc/ld.so.conf | wc -l` -eq 0 ]; then
            echo "$1" >> /etc/ld.so.conf
    fi

    ldconfig
}

undo_ld_path(){   
    for lib_path in openssl fbopenssl heimdal c-ares gsscred_cache fred_hash dns_cache
    do
        if [ `grep $lib_path /etc/ld.so.conf | wc -l` -eq 1 ]; then
            sed -i /$lib_path/d /etc/ld.so.conf
        fi
    done

    ldconfig
}

make_clean(){
    rm -rf packages/c-ares-$CARES_VER/
    rm -rf packages/fbopenssl-$FBOPENSSL_VER/
    rm -rf packages/fred_hash-$FRED_HASH_VER/
    rm -rf packages/dns_cache-$DNS_CACHE_VER/
    rm -rf packages/gsscred_cache-$GSSCRED_CACHE_VER/
    rm -rf packages/heimdal-$HEIMDAL_VER/
    rm -rf packages/openssl-$OPENSSL_VER/
    rm -rf packages/libevent-$LIBEVENT_VER/
    rm -rf packages/curl-$CURL_VER/
    rm -rf packages/hiredis-$HIREDIS_VER/
}

make_cleanall(){
    make_clean
    rm -rf $INSTALL_PATH
    undo_ld_path
}

cleanall_on_cl_version(){
    installed_ver_str=`curl-loader -V 2>&1|head -1`

    #Check if it support the command argument  -V
    if [ "${installed_ver_str:0:17}" = "Curl-loader 0.56." ]; then
        installed_ver_num=`echo "$installed_ver_str" | awk '{print $2}' | awk -F '.' '{print $3}'`
        fbopenssl_ver=`echo $FBOPENSSL_VER | awk -F"." '{print $NF}'`
        heimdal_ver=`echo $HEIMDAL_VER | awk -F"." '{print $NF}'`
        curl_ver=`echo $CURL_VER | awk -F"." '{print $NF}'`

        #To get the maxmium verison numbs of curl, heimdal, fbopenssl
        #max_ver_num=`echo "$heimdal_ver 109 $fbopenssl_ver 110 103"|cut -f1 -d" "|sort -n`
        max_ver_num=`echo $heimdal_ver $fbopenssl_ver $curl_ver | awk 'BEGIN {max=0} {for(i=1;i<=NF;i++) if ($i>max) max=$i fi} END {print max}'`
        #If the installed num is older(less than) to max_ver, need to clean installed libs
        if [ $installed_ver_num -lt $max_ver_num ]; then
            rm -rf $INSTALL_PATH
        fi
    else  #The version is less than 0.56.102, so need to clean installed libs
        rm -rf $INSTALL_PATH
    fi
}

compile_option(){    #$1:  debug switch
    if [ $1 -eq 1 ]; then
        FRED_HASH_OPT_FLAGS="debug=1"
        DNS_CACHE_OPT_FLAGS="debug=1"
        GSSCRED_CACHE_OPT_FLAGS="debug=1"
        OPENSSL_OPT_FLAGS="$DEBUG_FLAGS"
        HEIMDAL_OPT_FLAGS="$DEBUG_FLAGS"
        CARES_OPT_FLAGS="$OPT_FLAGS_WITH_DEBUG"
        LIBEVENT_OPT_FLAGS="$OPT_FLAGS_WITH_DEBUG"
        CURL_OPT_FLAGS="$OPT_FLAGS_WITH_DEBUG"
    else
        FRED_HASH_OPT_FLAGS=""
        DNS_CACHE_OPT_FLAGS=""
        GSSCRED_CACHE_OPT_FLAGS=""
        OPENSSL_OPT_FLAGS=""
        HEIMDAL_OPT_FLAGS=""
        CARES_OPT_FLAGS="$OPT_FLAGS_WITHOUT_DEBUG"
        LIBEVENT_OPT_FLAGS="$OPT_FLAGS_WITHOUT_DEBUG"
        CURL_OPT_FLAGS="$OPT_FLAGS_WITHOUT_DEBUG"
    fi
}

make_openssl(){
    if [ ! -f "$OPENSSL_LIB_PATH/libssl.so" ]; then
        rm -rf packages/openssl-$OPENSSL_VER/; rm -rf $OPENSSL_INSTALL_PATH; mkdir -p $OPENSSL_INSTALL_PATH;
        cd packages/; tar zxvf openssl-$OPENSSL_VER.tar.gz;
        cd openssl-$OPENSSL_VER/; 
        ./config --prefix=$OPENSSL_INSTALL_PATH no-rc2 no-md2 no-ssl2 no-idea no-mdc2 no-rc5 enable-tlsext \
                 no-krb5 threads shared;      #CFLAGS="$OPENSSL_OPT_FLAGS";
        make || exit 1; make install; config_ld_path $OPENSSL_LIB_PATH;
        cd ../..
    fi
}

make_fred_hash(){
    if [ ! -f "$FRED_HASH_LIB_PATH/libfredhash.a" ]; then
        rm -rf packages/fred_hash-$FRED_HASH_VER/; rm -rf $FRED_HASH_INSTALL_PATH; mkdir -p $FRED_HASH_INSTALL_PATH;
        cd packages/; tar jxvf fred_hash-$FRED_HASH_VER.tar.bz2;
        cd fred_hash-$FRED_HASH_VER/;
        make $FRED_HASH_OPT_FLAGS; make install; config_ld_path $FRED_HASH_LIB_PATH;
        cd ../..
    fi
}

make_dns_cache(){
    if [ ! -f "$DNS_CACHE_LIB_PATH/libdnscache.a" ]; then
        rm -rf packages/dns_cache-$DNS_CACHE_VER/; rm -rf $DNS_CACHE_INSTALL_PATH; mkdir -p $DNS_CACHE_INSTALL_PATH;
        cd packages/; tar jxvf dns_cache-$DNS_CACHE_VER.tar.bz2;
        cd dns_cache-$DNS_CACHE_VER/;
        make $DNS_CACHE_OPT_FLAGS; make install; config_ld_path $DNS_CACHE_LIB_PATH;
        cd ../..
    fi
}

make_gsscred_cache(){
    if [ ! -f "$GSSCRED_CACHE_LIB_PATH/libgsscredcache.a" ]; then
        rm -rf packages/gsscred_cache-$GSSCRED_CACHE_VER/; rm -rf $GSSCRED_CACHE_INSTALL_PATH; mkdir -p $GSSCRED_CACHE_INSTALL_PATH;
        cd packages/; tar jxvf gsscred_cache-$GSSCRED_CACHE_VER.tar.bz2;
        cd gsscred_cache-$GSSCRED_CACHE_VER/;
        make $GSSCRED_CACHE_OPT_FLAGS; make install; config_ld_path $GSSCRED_CACHE_LIB_PATH;
        cd ../..
    fi
}

make_heimdal(){
    if [ ! -f "$HEIMDAL_LIB_PATH/libkrb5.so" ]; then
        rm -rf packages/heimdal-$HEIMDAL_VER/; rm -rf $HEIMDAL_INSTALL_PATH; mkdir -p $HEIMDAL_INSTALL_PATH;
        cd packages/; tar jxvf heimdal-$HEIMDAL_VER.tar.bz2;
        cd heimdal-$HEIMDAL_VER/; 
        ./configure --prefix=$HEIMDAL_INSTALL_PATH --enable-pthread-support=yes --disable-sqlite-cache --disable-heimdal-documentation  \
                    --without-ipv6 --without-sqlite3 --without-openldap CFLAGS="$HEIMDAL_OPT_FLAGS"     \
                    LDFLAGS="-L$DNS_CACHE_LIB_PATH" LIBS="-ldnscache" CPPFLAGS="-I$DNS_CACHE_INCLUDE_PATH";
        make || exit 1; make install; config_ld_path $HEIMDAL_LIB_PATH;
        cd ../..
    fi
}

make_fbopenssl(){
    if [ ! -f "$FBOPENSSL_LIB_PATH/libfbopenssl.so" ]; then
        rm -rf packages/fbopenssl-$FBOPENSSL_VER/; rm -rf $FBOPENSSL_INSTALL_PATH; mkdir -p $FBOPENSSL_INSTALL_PATH;
        cd packages/; tar zxvf fbopenssl-$FBOPENSSL_VER.tar.gz;
        cd fbopenssl-$FBOPENSSL_VER/;
        make; make install; config_ld_path $FBOPENSSL_LIB_PATH;
        cd ../..
    fi
}

make_cares(){
    if [ ! -f "$CARES_LIB_PATH/libcares.a" ]; then
        rm -rf packages/c-ares-$CARES_VER/; rm -rf $CARES_INSTALL_PATH; mkdir -p $CARES_INSTALL_PATH;
        cd packages/; tar zxvf c-ares-$CARES_VER.tar.gz;
        cd c-ares-$CARES_VER/; ./buildconf; ./configure --prefix $CARES_INSTALL_PATH/ CFLAGS="$CARES_OPT_FLAGS";
        make; make install; config_ld_path $CARES_LIB_PATH;
        cd ../..
    fi
}

make_libevent(){
    if [ ! -f "$LIBEVENT_LIB_PATH/libevent.a" ]; then
        rm -rf packages/libevent-$LIBEVENT_VER/; rm -rf $LIBEVENT_INSTALL_PATH; mkdir -p $LIBEVENT_INSTALL_PATH;
        cd packages/; tar jxvf libevent-$LIBEVENT_VER.tar.bz2;
        cd libevent-$LIBEVENT_VER/; #patch -p1 < ../../patches/libevent-nevent.patch;
        ./configure --prefix $LIBEVENT_INSTALL_PATH --disable-thread-support --disable-openssl \
                    --enable-shared=no CFLAGS="$LIBEVENT_OPT_FLAGS";
        make; make install;
        cd ../..
    fi
}

make_curl(){
    if [ ! -f "$CURL_LIB_PATH/libcurl.a" ]; then
        rm -rf  packages/curl-$CURL_VER/; rm -rf $CURL_INSTALL_PATH; mkdir -p $CURL_INSTALL_PATH;
        cd packages/; tar jxvf curl-$CURL_VER.tar.bz2; patch -d curl-$CURL_VER -p1 < ../patches/curl-trace-info-error.patch;
        cd curl-$CURL_VER; ./buildconf; ./configure --prefix=$CURL_INSTALL_PATH --disable-file --disable-rtsp --disable-telnet --disable-sspi --disable-tftp   \
                           --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-manual --without-libmetalink --without-libidn   \
                           --without-librtmp --without-winssl --without-darwinssl --without-libssh2 --without-gnutls --without-polarssl --without-cyassl  \
                           --without-nss --without-axtls --disable-ldap --disable-ipv6 --enable-thread --with-random=/dev/urandom   \
                           --with-ssl=$OPENSSL_INSTALL_PATH  --with-gssapi=$HEIMDAL_LIB_PATH   \
                           --with-gssapi-includes=$HEIMDAL_INCLUDE_PATH --with-gssapi-libs=$HEIMDAL_LIB_PATH  \
                           --with-spnego=$FBOPENSSL_LIB_PATH --enable-shared=no --enable-ares=$CARES_INSTALL_PATH  \
                          LDFLAGS="-L$GSSCRED_CACHE_LIB_PATH" LIBS="-lgsscredcache"  \
                          CPPFLAGS="-I$CL_INCLUDE_PATH -DCURL_MAX_WRITE_SIZE=4096" CFLAGS="$CURL_OPT_FLAGS" ;
        make || exit 1; make install;
        cd ../..
    fi
}

make_hiredis(){
    if [ ! -f "$HIREDIS_LIB_PATH/libhiredis.a" ]; then
        rm -rf packages/hiredis-$HIREDIS_VER/; rm -rf $HIREDIS_INSTALL_PATH; mkdir -p $HIREDIS_INSTALL_PATH;
        cd packages/; tar jxvf hiredis-$HIREDIS_VER.tar.bz2;
        cd hiredis-$HIREDIS_VER; make static; make install;
        cd ../..
    fi
}

check_platform(){
    if [ $(uname -a | grep "x86_64" | wc -l) -eq 0 ]; then
        echo "Please install curl-loader on x86_64 platform."
        exit 1
    fi
}

usage(){
    echo "./install_deps.sh [OPTION]"
    echo ""
    echo "all_with_debug  		-	Install all the libs with debug flag on"
    echo "all_without_debug  		-	Install all the libs without debug flag on"
    echo "curl_with_debug  		-	Install libcurl with debug flag on"
    echo "curl_without_debug  		-	Install libcurl without debug flag on"
    echo "clean				-	Delete all the install files"
    echo "cleanall			-	Delete all the install files, lib files and undo ldconfig"
    echo "help				-	Print this message"
    echo ""
}

install_all(){    #$1: debug switch
    mkdir -p $CL_INCLUDE_PATH
    mkdir -p $CL_LIB_PATH
    mkdir -p $CL_OBJ_PATH

    check_platform
    compile_option $1

    cleanall_on_cl_version

    make_openssl
    make_fred_hash
    make_dns_cache
    make_heimdal
    make_gsscred_cache
    make_fbopenssl
    make_cares
    make_libevent
    make_hiredis
    copy_libs_and_incs

    make_curl
    cp -rf $CURL_LIB_PATH/lib*a $CL_LIB_PATH
    cp -rf $CURL_INCLUDE_PATH/* $CL_INCLUDE_PATH
}

install_libcurl(){    #$1: debug switch
    rm -rf CURL_INSTALL_PATH
    compile_option $1
    make_curl
    copy_libs_and_incs
}

############################################################################################################
if [ $# -eq 0 ]; then
    install_all 0
    exit
fi

if(($#>1)); then
    usage
    exit
fi

#check_platform

case "$1" in
    help) usage;;
    all_with_debug|1) install_all 1;;
    all_without_debug|0) install_all 0;;
    curl_with_debug) install_libcurl 1;;
    curl_without_debug) install_libcurl 0;;
    clean) make_clean;;
    cleanall) make_cleanall;;
    *) usage;;
esac
############################################################################################################

