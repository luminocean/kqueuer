require 'ffi'

class KQueuer
  module Native
    extend FFI::Library
    ffi_lib File.join(File.dirname(__FILE__) + '/native/kqueue.so')

    attach_function(:initialize, [], :void)
    attach_function(:ev_set, [:int, :int, :int], :void)
    attach_function(:ev_set, [:int, :int, :int], :void)
    attach_function(:wait, [], :int, :blocking => true)
    attach_function(:ready_fd, [:int], :int)
    attach_function(:fd_operation, [:int], :int)
  end

  # initialize native environment immediately
  Native.initialize


  KQ_READ = 1
  KQ_WRITE = 2

  def initialize
    @add_flag = 1
    @delete_flag = 2

    @memo = {}
  end

  def register(fd, operation, data)
    @memo[fd] ||= {}
    @memo[fd][operation] = data
    Native.ev_set(fd, operation, @add_flag)
  end

  def unregister(fd, operation)
    @memo[fd].delete(operation)
    @memo.delete(fd) if @memo[fd].keys.length == 0
    Native.ev_set(fd, operation, @delete_flag)
  end

  def wait
    nev = Native.wait

    events = (0...nev).map do |i|
      fd = Native.ready_fd(i)
      operation = Native.fd_operation(i)

      {
        :fd => fd,
        :operation => operation,
        :data => @memo[fd][operation]
      }
    end

    events
  end
end