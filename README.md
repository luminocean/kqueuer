KQueuer - A kqueue wrapper for Ruby

# 1. API

### register

```
# register a file and related operation you are interested in
# +fd+:: file descriptor
# +operation+:: operation you are interested in, currently supports KQueuer::KQ_READ and KQueuer::KQ_WRITE
# +data+:: arbitrary data that user specified, will be brought back without any modification once the event occurs
register(fd, operation, data)
```

### unregister

```
# unregister a file and related operation that are previously registered
# +fd+:: file descriptor
# +operation+:: operation you wew interested in
unregister(fd, operation)
```

Note that (fd, operation) tuple is used to identify an unique event by default.
So both fd and operation arguments are require to do the unregistering.

### wait

```
# wait for registered events to happen
wait()
```

`wait()` returns an array of events that occurred.

Structure of an event object looks like following:
```
{
    :fd => fd,
    :operation => operation,
    :data => data object you specified when the event was registered
}
```

Be careful that `wait()` may return an empty array.
`wait()` won't return without a reason.
If the returned events is empty it must be the case that there's nothing to be wait.
Remember to quit the waiting loop when this situation happens.

# 2. Example

The original file is `main.rb`. You can find it under the project root. I just copy it to here.
This script simple read data from STDIN and write the data back to STDOUT using underlying kqueue.

```
#! /usr/bin/env ruby

require_relative './kqueuer'

READ_SIZE = 1024 # how many bytes are read each time

kq = KQueuer.new

total_len = 0
data_to_write = []

# register for reading
kq.register(STDIN.fileno, KQueuer::KQ_READ, {
  :callback => lambda do
    #
    # refactor the string processing
    # dealing with the proportional writing
    #
    input = [] # byte array for input
    encoding = nil
    begin
      # read as much as it can
      # note that chunk is a string in its original binary format
      # so feel free to convert it into byte array using Array#bytes
      # without any unexpected changes
      # while (chunk = STDIN.read_nonblock(READ_SIZE)).length > 0
      #   encoding ||= chunk.encoding.name
      #   input += chunk.bytes
      # end

      chunk = STDIN.read_nonblock(READ_SIZE)
      encoding ||= chunk.encoding.name
      input += chunk.bytes
    rescue IO::EAGAINWaitReadable
      # done reading for this batch
    rescue EOFError
      kq.unregister(STDIN.fileno, KQueuer::KQ_READ)
    end

    # read nothing thus nothing to write, return
    return if input.length == 0

    data_to_write += input
    total_len += input.length

    # register for writing
    kq.register(STDOUT.fileno, KQueuer::KQ_WRITE, {
      :callback => lambda do
        len = STDOUT.write_nonblock(data_to_write.pack('C*').force_encoding(encoding))

        data_to_write = data_to_write.slice((len..-1))
        total_len -= len

        # nothing left to write
        if total_len == 0
          # unregister the write event after writing is done
          kq.unregister(STDOUT.fileno, KQueuer::KQ_WRITE)
        end
      end
    })
  end
})

while true
  events = kq.wait
  # nothing returned, done
  break if events.length == 0

  events.each do |ev|
    ev[:data][:callback].call
  end
end
```
