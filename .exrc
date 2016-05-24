if &cp | set nocp | endif
let s:cpo_save=&cpo
set cpo&vim
imap <silent> <expr> <F5> (pumvisible()?"\":"")."\\<Plug>LookupFile"
nmap  h
nmap <NL> j
nmap  k
nmap  l
nnoremap <silent>  :YRReplace '1', 'p'
nnoremap <silent>  :YRReplace '-1', 'P'
nmap  :BottomExplorerWindow
nmap  :FirstExplorerWindow
vnoremap <silent> # :call VisualSearch('b')
nnoremap ' "
vnoremap <silent> * :call VisualSearch('f')
map <silent> ,mm :ShowMarksPlaceMark
map <silent> ,ma :ShowMarksClearAll
map <silent> ,mh :ShowMarksClearMark
map <silent> ,mo :ShowMarksOn
map <silent> ,mt :ShowMarksToggle
nmap ,caL <Plug>CalendarH
nmap ,cal <Plug>CalendarV
map <silent> ,bv :VSBufExplorer
map <silent> ,bs :SBufExplorer
map <silent> ,be :BufExplorer
nmap ,ihn :IHN
nmap ,is :IHS:A
nmap ,ih :IHS
nnoremap ,' '
noremap ,dm mmHmn:%s///ge'nzt'm
nmap <silent> ,lv :lv /=expand("<cword>")/ %:lw
nmap ,cw :cw 10
nmap ,cp :cp
nmap ,cn :cn
vmap <silent> ,hr <Plug>MarkRegex
nmap <silent> ,hr <Plug>MarkRegex
vmap <silent> ,hh <Plug>MarkClear
nmap <silent> ,hh <Plug>MarkClear
vmap <silent> ,hl <Plug>MarkSet
nmap <silent> ,hl <Plug>MarkSet
nmap <silent> ,mk :MarksBrowser
nmap <silent> ,lw :LUWalk
nmap <silent> ,ll :LUBufs
nmap <silent> ,lk <Plug>LookupFile
nmap <silent> ,fe :Sexplore!
nmap <silent> ,wm :WMToggle
nmap <silent> ,tl :Tlist
map ,yr :YRShow
map ,s? z=
map ,sa zg
map ,sp [s
map ,sn ]s
map ,t4 :set shiftwidth=4
map ,t2 :set shiftwidth=2
vmap <silent> ,zo zO
nmap <silent> ,zo zO
map ,ec :tabnew ~/tmp/scratch.txt
map ,es :tabnew:setl buftype=nofile
nmap <silent> ,ws :call DeleteTrailingWS():w
map <silent> ,cd :cd %:p:h
map ,tm :tabmove
map ,tc :tabclose
map ,te :tabedit
map ,tn :tabnew
map ,bd :Bclose
nmap ,fu :se ff=unix
nmap ,fd :se ff=dos
map ,$ :syntax sync fromstart
map ,4 :set ft=javascript
map ,3 :set syntax=python
map ,2 :set syntax=xhtml
map ,1 :set syntax=c
map <silent> ,ee :call SwitchToBuf("~/.vimrc")
map <silent> ,ss :source ~/.vimrc
nmap <silent> ,rr :redraw!
nmap <silent> , :noh
nmap <silent> ,qa :qa
nmap <silent> ,qq :q
nmap <silent> ,qf :q!
nmap <silent> ,qw :wq
nmap <silent> ,wf :w!
nmap <silent> ,ww :w
nnoremap <silent> . :YRYankCount '.'
nnoremap ; :
vnoremap @w `>a"`<i"
vnoremap @q `>a'`<i'
vnoremap @$ `>a"`<i"
vnoremap @3 `>a}`<i{
vnoremap @2 `>a]`<i[
vnoremap @1 `>a)`<i(
imap ° 0i
imap ¤ $a
nnoremap <silent> D :YRYankCount 'D'
vnoremap <silent> P :YRPaste 'P', 'v'
nnoremap <silent> P :YRPaste 'P'
nnoremap <silent> Y :YRYankCount 'Y'
vmap [% [%m'gv``
vmap ]% ]%m'gv``
vmap a% [%v]%
vnoremap <silent> d :YRDeleteRange 'v'
nnoremap <silent> dgg :YRYankCount 'dgg'
nnoremap <silent> dG :YRYankCount 'dG'
nnoremap <silent> d$ :YRYankCount 'd$'
nnoremap <silent> daw :YRYankCount 'daw'
nnoremap <silent> diw :YRYankCount 'diw'
nnoremap <silent> dE :YRYankCount 'dE'
nnoremap <silent> de :YRYankCount 'de'
nnoremap <silent> dw :YRYankCount 'dw'
nnoremap <silent> dd :YRYankCount 'dd'
nmap gx <Plug>NetrwBrowseX
nnoremap <silent> gp :YRPaste 'gp'
nnoremap <silent> gP :YRPaste 'gP'
vnoremap <silent> p :YRPaste 'p', 'v'
nnoremap <silent> p :YRPaste 'p'
vnoremap <silent> y :YRYankRange 'v'
nnoremap <silent> ygg :YRYankCount 'ygg'
nnoremap <silent> yG :YRYankCount 'yG'
nnoremap <silent> y$ :YRYankCount 'y$'
nnoremap <silent> yaw :YRYankCount 'yaw'
nnoremap <silent> yiw :YRYankCount 'yiw'
nnoremap <silent> yE :YRYankCount 'yE'
nnoremap <silent> ye :YRYankCount 'ye'
nnoremap <silent> yw :YRYankCount 'yw'
nnoremap <silent> yy :YRYankCount 'yy'
nnoremap <silent> <Plug>NetrwBrowseX :call netrw#NetrwBrowseX(expand("<cWORD>"),0)
nmap <silent> <Plug>CalendarH :cal Calendar(1)
nmap <silent> <Plug>CalendarV :cal Calendar(0)
nmap <Nul>d :cs find d =expand("<cword>")
nmap <Nul>i :cs find i ^=expand("<cfile>")$
nmap <Nul>f :cs find f =expand("<cfile>")
nmap <Nul>e :cs find e =expand("<cword>")
nmap <Nul>t :cs find t =expand("<cword>")
nmap <Nul>c :cs find c =expand("<cword>")
nmap <Nul>g :cs find g =expand("<cword>")
nmap <Nul>s :cs find s =expand("<cword>")
nnoremap <silent> <F12> :A
nnoremap <silent> <F3> :Rgrep
map <F7> i=strftime("%Y-%m-%d %H:%M:%S")
cnoremap  <Home>
inoremap  
cnoremap  <End>
inoremap  
inoremap <expr> <NL> pumvisible()?"\<PageDown>\\":"\"
inoremap <expr>  pumvisible()?"\<PageUp>\\":"\"
cnoremap  
inoremap  
inoremap <expr>  pumvisible()?"\":"\"
inoremap <expr>  pumvisible()?"\":"\"
inoremap  
imap ,ihn :IHN
imap ,is :IHS:A
imap ,ih :IHS
cmap @vd vertical diffsplit 
inoremap @w "":let leavechar='"'i
inoremap @q '':let leavechar="'"i
inoremap @4 {o}:let leavechar="}"O
inoremap @3 {}:let leavechar="}"i
inoremap @2 []:let leavechar="]"i
inoremap @1 ():let leavechar=")"i
iabbr xname Easwy Yang
iabbr xdate =strftime("%c")
let &cpo=s:cpo_save
unlet s:cpo_save
set autoindent
set autoread
set background=dark
set backspace=eol,start,indent
set cindent
set cmdheight=2
set complete=.,w,b,t
set completeopt=menu
set cscopeprg=/usr/bin/cscope
set cscopetag
set cscopetagorder=1
set cscopeverbose
set diffopt=filler,vertical
set expandtab
set fileencodings=utf-8,gbk,ucs-bom,cp936
set grepprg=grep\ -nH\ $*
set helplang=cn
set history=400
set hlsearch
set ignorecase
set incsearch
set laststatus=2
set lazyredraw
set printoptions=paper:a4
set ruler
set runtimepath=~/.vim,/var/lib/vim/addons,/usr/share/vim/vimfiles,/usr/share/vim/vim73,/usr/share/vim/vimfiles/after,/var/lib/vim/addons/after,~/.vim/after
set sessionoptions=blank,buffers,folds,help,options,tabpages,winsize,sesdir
set shiftwidth=4
set smartindent
set smarttab
set suffixes=.bak,~,.swp,.o,.info,.aux,.log,.dvi,.bbl,.blg,.brf,.cb,.ind,.idx,.ilg,.inx,.out,.toc
set switchbuf=useopen
set tabstop=4
set tags=/usr/include/tags,~/linux-2.6.30.4/tags
set termencoding=utf-8
set viminfo='10,\"100,:20,n~/.viminfo
set whichwrap=b,s,<,>
set wildmenu
set nowritebackup
" vim: set ft=vim :
