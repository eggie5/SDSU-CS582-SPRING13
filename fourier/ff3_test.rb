require './fft3.rb'
require 'test/unit'

class TestDFT < Test::Unit::TestCase
        def setup
                # Compose a data set containing two frequencies, f1 & f2
                # Assume the data set contains 1 second worth of samples so
                # f1 & f2 are in Hz and should be less than half the number of samples (Nyquist)
                @data = Array.new(64)
                f1 =10
                f2 =25
                amplitude = 1
                i=-1
                @data.map! do |d|
                        i=i+1
                        Math.sin( 2 * Math::PI * f1 * i / @data.size ) + Math.sin( 2 * Math::PI * f2 * i / @data.size )
                end
        end

        def test_dft_throws_exception_with_no_input
                dft = DFT.new( 32 )
                assert_raise RuntimeError do
                        freq = dft.transform
                end
        end

        def test_dft_has_correct_output_size
                dft = DFT.new( 32 )
                dft.data( @data )
                freq = dft.transform
                assert_equal( 32, freq.size )
                assert_equal( 32, dft.freq.size )
        end

        def test_dft_has_correct_output
                dft = DFT.new( 32 )
                dft.data( @data )
                dft.transform

                assert dft.freq.at(10).abs > 1
                assert dft.freq.at(25).abs > 1
        end

        def test_display
                dft = DFT.new( 32 )
                dft.data( @data )
                dft.transform
                puts "\nFrequencies found:\n"
                dft.freq.each_with_index do |f,i|
                        if (f.abs > 1)
                                printf( "%d %.30f   %.30f  %.30f\n", i, f.real, f.imag, f.abs )
                        end
                end
        end
end