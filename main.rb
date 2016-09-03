#! /usr/bin/env ruby

require_relative './kqueuer'

READ_SIZE = 1024 # how many bytes are read each time

kq = KQueuer.new
# register for reading
kq.register(STDIN.fileno, KQueuer::KQ_READ, {
  :callback => lambda do
    begin
      # read as much as it can
      input = STDIN.read_nonblock(READ_SIZE)
    rescue EOFError
      kq.unregister(STDIN.fileno, KQueuer::KQ_READ)
      return # done reading
    end

    total_len = input.bytes.length # byte length in UTF-8
    # register for writing
    kq.register(STDOUT.fileno, KQueuer::KQ_WRITE, {
      :callback => lambda do
        len = STDOUT.write_nonblock(input)
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


