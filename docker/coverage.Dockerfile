### Build/test container ###
# Define builder stage
FROM new-grad-ten-years-experience:base as builder

ARG project=/usr/src/projects/new-grad-ten-years-experience

# Install git
RUN apt-get update && apt-get install -y git

# Share work directory
COPY . ${project}

WORKDIR ${project}

# Initialize submodules
RUN git init
RUN git add -f .
RUN git submodule add https://github.com/commonmark/cmark.git external/cmark
RUN git status
RUN git -c user.name='new-grad' -c user.email='new-grad@cs130.org' commit -m "commit files"

RUN git submodule update --init --recursive

WORKDIR ${project}/build_coverage

RUN cmake -DCMAKE_BUILD_TYPE=Coverage ..
RUN make coverage
