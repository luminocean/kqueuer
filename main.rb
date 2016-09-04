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
      STDERR.write "EOF\n"
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


