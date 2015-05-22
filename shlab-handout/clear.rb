def clear(a)
	File.foreach(a) do |line|
		if not line.include? "./sdriver.pl -t"
			line = line.sub /\(+[[:digit:]]+\)/, ''
			puts(line)
		end
	end
end

clear ARGV[0]

