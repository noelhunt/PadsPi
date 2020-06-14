	sed -e 's/\\f(..//g' -e 's/\\f[a-z0-9A-Z]//g' -e 's/\\em//' -e 's/ ,/,/' -e 's/\\\&//g' -e 's/,/	/' -e 's/^ //' -e 's/  / /' x|sed 's/	 \([^A-Za-z]\)/	  \1/'|sort -f|sed -e 's/	  /	 /' -e 's/\b//'|
	awk  '
	#single
	#s
	#Sfor cw
	#p for 1cw
	#r for 2cw
	#t for 4cw
	#q for 3cw
	#x for 4&6
	#y for 1&3
	#z for 2&4
	#Z for 3&5
	#Y for 1,2,&4
	#
	#global
	#g
	#G for CW
	#P for 1cw
	#R for 2cw
	#
	#c cw
	#f 1st cw
	#C paper name to cw
	BEGIN	{
		while((getline<"contents") > 0){
			names[$1] = $2
			last[$1] = $3
		}
		FS="	"
	}
	{
		junk=$0
		caps=gsub(/[A-HJ-Z]/,"",junk)
		constant=0
		control=0
		if($1 == term){
		ck = global = substr($2,2,index($2,",")-2)
		if(ck in names && ((NF>2 && $NF !~ /C/)||NF < 3)){
			sub(/^ /, " \\fI", $2)
			sub(/,/,"\\fP,", $2)
		}
		if($2 ~ /[A-Z_$][A-Z_][A-Z_$0-9& ]*/){
			if($2 !~ /[a-z]/)caps = caps/2
			control=8*gsub(/[A-Z_$][A-Z_$][A-Z_$0-9& ]*/,"\\s-2&\\s+2")
		}
		sub(/^ /,"",$2)
		if(NF == 3){
			rside = ckcw($2,0,$NF)
			if(global in names){
			if( $3~ /[gGPR]/)
			printit("    \\fI" global "\\fP, " names[global] "-" last[global],1,control)
			else if($2 ~ /\\fI/)printit("    " $2, 1, control)
			else printit("    " rside, 1, control)
				next
			}
		}
		else rside = $2
		printit("    " rside, 1, control)	#removed space
	}
	else {
		term = $1
		control=0
		if(hadamatch){
			printf("\n")
		}
		if(term in names){
			if(!hadamatch)printf("\n")
			hadamatch=1
			newt="\\fI" term "\\fP, " names[term] "-" last[term]
			printit(newt, 0, 0)
			if($2 ~ /[A-Z_$][A-Z_$][A-Z_$0-9& ]*/){
				if($2 !~ /[a-z]/)caps /= 2
				control=8*gsub(/[A-Z_$][A-Z_$][A-Z_$0-9& ]*/,"\\s-2&\\s+2")
			}
			sub(/^ /,"",$2)
			if(NF == 3)
				rside = ckcw($2,0,$NF)
			else rside = $2
			printit("    " rside, 1, control)	#removed space
		}
		else {
			hadamatch=0
			if($1 ~ /[A-Z_$][A-Z_$][A-Z_$0-9& ]*/){
				control=8*gsub(/[A-Z_$][A-Z_$][A-Z_$0-9& ]*/,"\\s-2&\\s+2")
			}
			lside = $1
			if(NF == 3){
				lside = ckcw($1,1,$NF)
				if($NF ~ /[gGPR]/){
				r = substr($2,2,index($2,",")-2)
				if(r in names ){
					control += 6
					printit(lside ", \\fI" r "\\fP, " names[r] "-" last[r],0,control)
					next
				}
				else {
					print "term not in names:" r|"cat 1>&2"
					sub(/	./,"")
				}
				}
			}
			control += 6
			sub(",","\\fP,",$2)
			printit(lside ",\\fI" $2, 0, control)
		}
	}}
	function ckcw(rterm,first,code){
		constant=0
		savit=rterm
		if(rterm ~ /^-/){
			control += 2
			sub(/^-/,"\\(em",rterm)
		}
		if(rterm ~ /^\./){
			control += 2
			sub(/^\./,"\\\\&&",rterm)
		}
		if(code ~ /[gs]/)return(rterm)
		if(code ~ /[GcSC]/){
			if(first)rterm = addit(rterm)
			else {
				constant = index(rterm,",")-1
				if(rterm ~ /s[-+]2/){
					constant -= 8
					gsub(/s\+2/,"s+1", rterm)
					gsub(/s\-2/,"s-1", rterm)
					rterm = "\\s-1\\f(CW" rterm
					sub(/,/,"\\fP\\s+1,",rterm)
				}
				else {
					rterm = "\\s-1\\f(CW" rterm
					sub(/,/,"\\fP\\s+1,",rterm)
				}
			}
		}
		else if(code ~ /[pPf]/){
			constant = index(rterm," ")-1
			if(rterm ~ /s[-+]2/){
				constant -= 8
				gsub(/s\+2/,"s+1", rterm)
				gsub(/s\-2/,"s-1", rterm)
				rterm = "\\s-1\\f(CW" rterm
				sub(/ /,"\\fP\\s+1 ", rterm)
			}
			else {
				rterm = "\\s-1\\f(CW" rterm
				sub(/ /,"\\fP\\s+1 ", rterm)
			}
		}
		else{
			n=split(rterm, arr, " ")
			if(code ~ /[Yy]/)rterm = addit(arr[1])
			else rterm = arr[1]
			for(i=2;i<=n;i++){
				if(i==2 && code~/[rRzY]/)rterm = rterm " " addit(arr[i])
				else if(i==3 && code~/[qyZ]/)rterm=rterm " " addit(arr[i])
				else if(i==4 && code~/[txzY]/)rterm=rterm " " addit(arr[i])
				else if(i==5 && code~/[Z]/)rterm=rterm " " addit(arr[i])
				else if(i==6 && code~/x/)rterm=rterm " " addit(arr[i])
				else {
					if(arr[i] ~ /s\+2/)sub(/s\+2/,"s+1",arr[i])
					rterm = rterm " " arr[i]
				}
			}
		}
		if(code ~ /[xyz]/)control+=16
		control += 16
		return(rterm)
	}
	function addit(string){
		constant += length(string)
		if(string ~ /s[-+]2/){
			constant -= 8
			gsub(/s\+2/,"s+1", string)
			gsub(/s\-2/,"s-1", string)
			return("\\s-1\\f(CW" string "\\fP\\s+1")
		}
		else return("\\s-1\\f(CW" string "\\fP\\s+1")
	}
	function printit(line, space, control){
	if(!constant)
		limit=43+control - caps*.5			#was 46 at 8 pt - 43 with italic change
	else limit = 43 - constant*.4 + 1 + control - caps*.5
	print "lim " limit " leng " length(line) " cons:ctl:caps " constant ":" control ":" caps " " line|"cat 1>&2"
	if(space == 0)limit += 2
	if(length(line)< limit)print line
	else {
		n=split(line, a, ",")
		if(length(a[1])>=limit-6 || space){			#adjust for italic change
			if(!space)limit -=6
			m=split(a[1],b," ")
			nline=b[1]
			if(space)nline = "    " b[1]
			if(m > 1)for(i=2;i<=m;i++){
				if(length(nline)+length(b[i])+1>=limit){
					if(nline ~ /s-2/ && nline !~ /s+2/){
						print nline "\\s+2"
						nline = "      \\s-2" b[i]
					}
					else {
						print nline
						nline = "      " b[i]
					}
				}
				else nline = nline " " b[i]
			}
			for(i=2; i<=n;i++){
				leng=length(a[i])
				if(a[i] ~ /-/)leng--
				if(length(nline)+leng >= limit){
					print nline ","
					nline = "      " a[i]
				}
				else nline = nline "," a[i]
			}
			print nline
			return
		}
		nline=a[1]
		for(i=2;i<=n;i++){
			if(length(nline)+ length(a[i])>= limit){	#used to be -6
				print nline ","
				nline = "      " a[i]
			}
			else nline = nline "," a[i]
		}
		if(length(nline) > 0)print nline
	}
	}' 
