### Build/test container ###
# Define builder stage
FROM new-grad-ten-years-experience:base as builder

# Install git
RUN apt-get update && apt-get install -y git

ARG project=/usr/src/projects/new-grad-ten-years-experience

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

WORKDIR ${project}/build

# Build and test
RUN cmake ..
RUN make
RUN find . -name "server"
RUN ctest --output-on_failure

### Deploy container ###
# Define deploy stage
FROM ubuntu:jammy as deploy

ARG project=/usr/src/projects/new-grad-ten-years-experience

COPY --from=builder ${project}/build/bin/server .
COPY --from=builder ${project}/configs/deploy.conf .
COPY --from=builder ${project}/static /files/static

EXPOSE 80

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./server"]

# Use CMD to specify arguments to ENTRYPOINT
CMD ["deploy.conf"]