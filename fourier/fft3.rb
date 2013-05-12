require 'complex'

class DFT
        def initialize( size )
                @freq = Array.new( size, Complex(0,0) )
        end

        # set data
        def data( data )
                @data = data
        end

        # get frequency table
        def freq
                @freq
        end

        # reset the frequency table
        def reset
                @freq.map! do |d|
                        Complex( 0,0 )
                end
        end

        # transform data and return the frequency table
        def transform
                raise "No input data" unless @data
                (0..@freq.size-1).each do |k|
                        @data.each_with_index do |d,i|
                                @freq[k] = Complex(
                                        @freq[k].real + d * Math.cos( 2 * Math::PI * k * i / @data.size ),
                                        @freq[k].imag - d * Math.sin( 2 * Math::PI * k * i / @data.size )
                                )
                        end
                end
                return @freq
        end
end