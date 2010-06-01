def readFile(fileName):
	fin = open(fileName)
	header = None
	d = {}
	for line in fin:
		if not header:
			header = line
		else:
			fields = line.split()
			word = fields[0]
			count = int(fields[1])
			d[word] = count
	return header, d

def writeFile(fileName, header, d, n):
	fout = open(fileName, 'w')	
	fout.write(header)
	l = d.items()
	l.sort(lambda x, y: cmp(y[1], x[1]))
	l = l[:n]
	l.sort()
	for word, count in l:
		fout.write('%s\t%d\n' % (word, count))

if __name__ == '__main__':
	header, d = readFile('col_stats')
	writeFile('col_stats_small', header, d, 10000)
