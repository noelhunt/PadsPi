/* sccsid[] = "%W%\t%G%" */

class Wd : public PadRcv {
	long	key;
	Pad	*pad;
const	char	*getwd;
const	char	*prevwd;
	Index	ix;
	Index	carte();
	void	pwd(Attrib=0);
public:
		Wd();
	char	*kbd(char *);
	char	*help();
const	char	*enchiridion(long);
};
