#!/bin/bash

GIT_HASH=$1

if [ $# -eq 0 ]; then
    GIT_HASH='master'
fi

DATA_REPO=${FIRERS_DATA:-ssh://git@redmine.laas.fr/laas/users/simon/fire-rs/fire-rs-data.git}
CODE_REPO="`pwd`/../.."

# create a directory in which to run the benchmark
DIR="saop_${GIT_HASH}"
mkdir -p ${DIR}

echo "Workdir: ${DIR}"
cd ${DIR}

    # fetch clean data and code repository (non commited changes are ignored!)
    echo "Fetching repositories"
    if test -d ${DATA_REPO}; then
        echo "Creating "$(pwd)"/data"
        mkdir -p data
        cd data
            echo "Making a symbolics link to ${DATA_REPO}"
            if ! test -d dem; then
                if ! ln --symbolic ${DATA_REPO}/dem; then
                    echo "Cannot create a symbolic link to $DATA_REPO/dem"
                    exit 1
                fi
            fi
            if ! test -d wind; then
                if ! ln --symbolic ${DATA_REPO}/wind; then
                    echo "Cannot create a symbolic link to ${DATA_REPO}/wind"
                    exit 2
                fi
            fi
            if ! test -d landcover; then
                if ! ln --symbolic ${DATA_REPO}/landcover; then
                    echo "Cannot create a symbolic link to ${DATA_REPO}/landcover"
                    exit 3
                fi
            fi
        cd ..
    else
        echo "FIRERS_DATA is not defined, trying to clone from "${DATA_REPO}
        if ! git clone ${DATA_REPO} data; then
            echo "'git clone' failed! exiting..."
            exit 1
        fi
    fi

    git clone ${CODE_REPO} fire-rs-saop || true
    cd fire-rs-saop
        git submodule update --init --recursive
        git pull
        git checkout ${GIT_HASH}
    cd ..

    # start container in the background
    echo "Starting docker container 'saop'"
    USER_ID="saop"
    GROUP_ID="saop"
    docker run -d -it --user=${USER_ID}:${GROUP_ID} --name=saop \
           -v `pwd`/fire-rs-saop:/home/saop/code:z \
           -v `pwd`/data:/home/saop/data:z \
           -v `pwd`/data/dem:/home/saop/data/dem:z \
           -v `pwd`/data/wind:/home/saop/data/wind:z \
           -v `pwd`/data/landcover:/home/saop/data/landcover:z \
           saop

    # display running containers
    docker ps
cd ..
