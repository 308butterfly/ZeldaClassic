//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  sprite.cc
//
//  Sprite classes:
//   - sprite:      base class for the guys and enemies in zelda.cc
//   - movingblock: the moving block class
//   - sprite_list: main container class for different groups of sprites
//   - item:        items class
//
//--------------------------------------------------------

#include "precompiled.h" //always first

#include "sprite.h"
#include "decorations.h"
#include "zc_custom.h"
#include "zelda.h"
#include "maps.h"
#include "zsys.h"
#include "hero.h"

/***************************************/
/*******  Decoration Base Class  *******/
/***************************************/

decoration::decoration(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : sprite()
{
	x=X;
	y=Y;
	id=Id;
	clk=Clk;
	misc = 0;
	yofs = (get_bit(quest_rules, qr_OLD_DRAWOFFSET)?playing_field_offset:original_playing_field_offset) - 2;
	the_deco_sprite = vbound(wpnSpr,0,255);
}

decoration::~decoration() {}

/*******************************/
/*******   Decorations   *******/
/*******************************/

int32_t dBushLeaves::ft[4][8][3];
int32_t dFlowerClippings::ft[4][8][3];
int32_t dGrassClippings::ft[3][4][4];
int32_t dHammerSmack::ft[2][4][3];

dBushLeaves::dBushLeaves(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=6;
		ft[0][0][1]=6;
		ft[0][0][2]=0;
		ft[0][1][0]=6;
		ft[0][1][1]=9;
		ft[0][1][2]=0;
		ft[0][2][0]=7;
		ft[0][2][1]=11;
		ft[0][2][2]=1;
		ft[0][3][0]=7;
		ft[0][3][1]=13;
		ft[0][3][2]=0;
		ft[0][4][0]=9;
		ft[0][4][1]=15;
		ft[0][4][2]=0;
		ft[0][5][0]=10;
		ft[0][5][1]=16;
		ft[0][5][2]=0;
		ft[0][6][0]=11;
		ft[0][6][1]=18;
		ft[0][6][2]=0;
		ft[0][7][0]=4;
		ft[0][7][1]=19;
		ft[0][7][2]=1;
		
		ft[1][0][0]=-4;
		ft[1][0][1]=3;
		ft[1][0][2]=0;
		ft[1][1][0]=-1;
		ft[1][1][1]=4;
		ft[1][1][2]=0;
		ft[1][2][0]=0;
		ft[1][2][1]=5;
		ft[1][2][2]=0;
		ft[1][3][0]=1;
		ft[1][3][1]=5;
		ft[1][3][2]=1;
		ft[1][4][0]=-2;
		ft[1][4][1]=5;
		ft[1][4][2]=1;
		ft[1][5][0]=-6;
		ft[1][5][1]=5;
		ft[1][5][2]=0;
		ft[1][6][0]=-7;
		ft[1][6][1]=4;
		ft[1][6][2]=0;
		ft[1][7][0]=-9;
		ft[1][7][1]=2;
		ft[1][7][2]=1;
		
		ft[2][0][0]=10;
		ft[2][0][1]=2;
		ft[2][0][2]=1;
		ft[2][1][0]=7;
		ft[2][1][1]=3;
		ft[2][1][2]=1;
		ft[2][2][0]=4;
		ft[2][2][1]=5;
		ft[2][2][2]=1;
		ft[2][3][0]=5;
		ft[2][3][1]=5;
		ft[2][3][2]=0;
		ft[2][4][0]=8;
		ft[2][4][1]=9;
		ft[2][4][2]=0;
		ft[2][5][0]=9;
		ft[2][5][1]=9;
		ft[2][5][2]=0;
		ft[2][6][0]=12;
		ft[2][6][1]=9;
		ft[2][6][2]=1;
		ft[2][7][0]=6;
		ft[2][7][1]=16;
		ft[2][7][2]=0;
		
		ft[3][0][0]=4;
		ft[3][0][1]=-4;
		ft[3][0][2]=0;
		ft[3][1][0]=4;
		ft[3][1][1]=-6;
		ft[3][1][2]=0;
		ft[3][2][0]=2;
		ft[3][2][1]=-7;
		ft[3][2][2]=0;
		ft[3][3][0]=1;
		ft[3][3][1]=-8;
		ft[3][3][2]=3;
		ft[3][4][0]=0;
		ft[3][4][1]=-9;
		ft[3][4][2]=0;
		ft[3][5][0]=-1;
		ft[3][5][1]=-11;
		ft[3][5][2]=0;
		ft[3][6][0]=-2;
		ft[3][6][1]=-14;
		ft[3][6][2]=0;
		ft[3][7][0]=-3;
		ft[3][7][1]=-16;
		ft[3][7][2]=0;
	}
}

bool dBushLeaves::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	return (clk++>=24);
}

void dBushLeaves::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	int32_t t=0;
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].newtile;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwBushLeaves].newtile;
		cs=wpnsbuf[iwBushLeaves].csets&15;
		
	}
	
	for(int32_t i=0; i<4; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		flip=ft[i][int32_t(float(clk-1)/3)][2];
		tile=t*4+i;
		decoration::draw8(dest);
	}
}


comboSprite::comboSprite(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) :
	decoration(X,Y,Id,Clk), timer(0), initialized(false)
{
	id=Id;
	clk=Clk;
	
	the_deco_sprite = vbound(wpnSpr,0,255);
	tframes = wpnsbuf[the_deco_sprite].frames;
	spd = wpnsbuf[the_deco_sprite].speed;
}

bool comboSprite::animate(int32_t index)
{
	/*
	clk++;
	if ( (clk%spd) == 0 ) tile++;
	al_trace("Animating combo sprite.z\n");
	//if ( clk > tframes ) dead=1;
	return clk()<=0;
	*/
	int32_t dur = zc_max(1,wpnsbuf[the_deco_sprite].frames) * zc_max(1,wpnsbuf[the_deco_sprite].speed);
	//al_trace("dur: %d\n", dur);
	//al_trace("clk: %d\n", clk);
	return (clk++>=dur);
}
/*
void comboSprite::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	al_trace("Drawing combo sprite.z\n");
	tile=wpnsbuf[the_deco_sprite].newtile;
	cs=wpnsbuf[the_deco_sprite].csets&15;
	sprite::draw(dest);
}
*/
void comboSprite::realdraw(BITMAP *dest, int32_t draw_what)
{
	if(misc!=draw_what)
	{
		return;
	}
	
	int32_t fb=the_deco_sprite;
	int32_t t=wpnsbuf[fb].newtile;
	int32_t fr=wpnsbuf[fb].frames;
	int32_t spd=wpnsbuf[fb].speed;
	cs=wpnsbuf[fb].csets&15;
	flip=0;
	
		tile=t;
		
		if(fr>0&&spd>0)
		{
			tile+=((clk/spd)%fr);
		}
		
		decoration::draw(dest);
		
}

void comboSprite::draw(BITMAP *dest)
{
	realdraw(dest,0);
}

void comboSprite::draw2(BITMAP *dest)
{
	realdraw(dest,1);
}

dFlowerClippings::dFlowerClippings(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=6;
		ft[0][0][1]=6;
		ft[0][0][2]=0;
		ft[0][1][0]=6;
		ft[0][1][1]=9;
		ft[0][1][2]=0;
		ft[0][2][0]=7;
		ft[0][2][1]=11;
		ft[0][2][2]=1;
		ft[0][3][0]=7;
		ft[0][3][1]=13;
		ft[0][3][2]=0;
		ft[0][4][0]=9;
		ft[0][4][1]=15;
		ft[0][4][2]=0;
		ft[0][5][0]=10;
		ft[0][5][1]=16;
		ft[0][5][2]=0;
		ft[0][6][0]=11;
		ft[0][6][1]=18;
		ft[0][6][2]=0;
		ft[0][7][0]=4;
		ft[0][7][1]=19;
		ft[0][7][2]=1;
		
		ft[1][0][0]=-4;
		ft[1][0][1]=3;
		ft[1][0][2]=0;
		ft[1][1][0]=-1;
		ft[1][1][1]=4;
		ft[1][1][2]=0;
		ft[1][2][0]=0;
		ft[1][2][1]=5;
		ft[1][2][2]=0;
		ft[1][3][0]=1;
		ft[1][3][1]=5;
		ft[1][3][2]=1;
		ft[1][4][0]=-2;
		ft[1][4][1]=5;
		ft[1][4][2]=1;
		ft[1][5][0]=-6;
		ft[1][5][1]=5;
		ft[1][5][2]=0;
		ft[1][6][0]=-7;
		ft[1][6][1]=4;
		ft[1][6][2]=0;
		ft[1][7][0]=-9;
		ft[1][7][1]=2;
		ft[1][7][2]=1;
		
		ft[2][0][0]=10;
		ft[2][0][1]=2;
		ft[2][0][2]=1;
		ft[2][1][0]=7;
		ft[2][1][1]=3;
		ft[2][1][2]=1;
		ft[2][2][0]=4;
		ft[2][2][1]=5;
		ft[2][2][2]=1;
		ft[2][3][0]=5;
		ft[2][3][1]=5;
		ft[2][3][2]=0;
		ft[2][4][0]=8;
		ft[2][4][1]=9;
		ft[2][4][2]=0;
		ft[2][5][0]=9;
		ft[2][5][1]=9;
		ft[2][5][2]=0;
		ft[2][6][0]=12;
		ft[2][6][1]=9;
		ft[2][6][2]=1;
		ft[2][7][0]=6;
		ft[2][7][1]=16;
		ft[2][7][2]=0;
		
		ft[3][0][0]=4;
		ft[3][0][1]=-4;
		ft[3][0][2]=0;
		ft[3][1][0]=4;
		ft[3][1][1]=-6;
		ft[3][1][2]=0;
		ft[3][2][0]=2;
		ft[3][2][1]=-7;
		ft[3][2][2]=0;
		ft[3][3][0]=1;
		ft[3][3][1]=-8;
		ft[3][3][2]=3;
		ft[3][4][0]=0;
		ft[3][4][1]=-9;
		ft[3][4][2]=0;
		ft[3][5][0]=-1;
		ft[3][5][1]=-11;
		ft[3][5][2]=0;
		ft[3][6][0]=-2;
		ft[3][6][1]=-14;
		ft[3][6][2]=0;
		ft[3][7][0]=-3;
		ft[3][7][1]=-16;
		ft[3][7][2]=0;
	}
}

bool dFlowerClippings::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	return (clk++>=24);
}

void dFlowerClippings::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	
	int32_t t=0;
	
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].newtile;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwFlowerClippings].newtile;
		cs=wpnsbuf[iwFlowerClippings].csets&15;
		
	}
	
	
	for(int32_t i=0; i<4; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		flip=ft[i][int32_t(float(clk-1)/3)][2];
		tile=t*4+i;
		decoration::draw8(dest);
	}
}

dGrassClippings::dGrassClippings(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=1;
		ft[0][0][1]=0;
		ft[0][0][2]=1;
		ft[0][0][3]=0;
		ft[0][1][0]=-1;
		ft[0][1][1]=4;
		ft[0][1][2]=1;
		ft[0][1][3]=0;
		ft[0][2][0]=-5;
		ft[0][2][1]=2;
		ft[0][2][2]=0;
		ft[0][2][3]=1;
		ft[0][3][0]=-8;
		ft[0][3][1]=0;
		ft[0][3][2]=0;
		ft[0][3][3]=1;
		
		ft[1][0][0]=3;
		ft[1][0][1]=-4;
		ft[1][0][2]=0;
		ft[1][0][3]=1;
		ft[1][1][0]=8;
		ft[1][1][1]=-5;
		ft[1][1][2]=3;
		ft[1][1][3]=1;
		ft[1][2][0]=8;
		ft[1][2][1]=-5;
		ft[1][2][2]=3;
		ft[1][2][3]=0;
		ft[1][3][0]=8;
		ft[1][3][1]=-5;
		ft[1][3][2]=0;
		ft[1][3][3]=1;
		
		ft[2][0][0]=8;
		ft[2][0][1]=1;
		ft[2][0][2]=0;
		ft[2][0][3]=1;
		ft[2][1][0]=10;
		ft[2][1][1]=4;
		ft[2][1][2]=1;
		ft[2][1][3]=1;
		ft[2][2][0]=15;
		ft[2][2][1]=3;
		ft[2][2][2]=0;
		ft[2][2][3]=0;
		ft[2][3][0]=16;
		ft[2][3][1]=5;
		ft[2][3][2]=0;
		ft[2][3][3]=1;
	}
}

bool dGrassClippings::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	return (clk++>=12);
}

void dGrassClippings::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	
	int32_t t=0;
	
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].newtile;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwGrassClippings].newtile;
		cs=wpnsbuf[iwGrassClippings].csets&15;
		
	}
	
	for(int32_t i=0; i<3; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		flip=ft[i][int32_t(float(clk-1)/3)][2];
		tile=(t+(ft[i][int32_t(float(clk-1)/3)][3]))*4+i;
		decoration::draw8(dest);
	}
}

dHammerSmack::dHammerSmack(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	ox=X;
	oy=Y;
	id=Id;
	clk=Clk;
	the_deco_sprite = vbound(wpnSpr,0,255);
	static bool initialized=false;
	if(!initialized)
	{
		initialized=true;
		
		ft[0][0][0]=-5;
		ft[0][0][1]=-4;
		ft[0][0][2]=0;
		ft[0][1][0]=-4;
		ft[0][1][1]=-3;
		ft[0][1][2]=1;
		ft[0][2][0]=-5;
		ft[0][2][1]=-4;
		ft[0][2][2]=1;
		ft[0][3][0]=-8;
		ft[0][3][1]=-7;
		ft[0][3][2]=1;
		
		ft[1][0][0]=3;
		ft[1][0][1]=-4;
		ft[1][0][2]=0;
		ft[1][1][0]=5;
		ft[1][1][1]=-3;
		ft[1][1][2]=1;
		ft[1][2][0]=6;
		ft[1][2][1]=-4;
		ft[1][2][2]=1;
		ft[1][3][0]=9;
		ft[1][3][1]=-7;
		ft[1][3][2]=1;
	}
	
	wpnid=itemsbuf[current_item_id(itype_hammer)].wpn2;
}

bool dHammerSmack::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	return (clk++>=12);
}

void dHammerSmack::draw(BITMAP *dest)
{
	if(screenscrolling)
	{
		clk=128;
		return;
	}
	
	int32_t t=wpnsbuf[wpnid].newtile;
	cs=wpnsbuf[wpnid].csets&15;
	flip=0;
	
	for(int32_t i=0; i<2; ++i)
	{
		x=ox+ft[i][int32_t(float(clk-1)/3)][0];
		y=oy+ft[i][int32_t(float(clk-1)/3)][1];
		tile=t*4+i+(ft[i][int32_t(float(clk-1)/3)][2]*2);
		decoration::draw8(dest);
	}
}

dTallGrass::dTallGrass(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
}

bool dTallGrass::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	
	if(HeroZ()>8) return true;
	bool g1 = isGrassType(COMBOTYPE(HeroX(),HeroY()+15)), g2 = isGrassType(COMBOTYPE(HeroX()+15,HeroY()+15));
	if(get_bit(quest_rules, qr_BUSHESONLAYERS1AND2))
	{
		g1 = g1 || isGrassType(COMBOTYPEL(1,HeroX(),HeroY()+15)) || isGrassType(COMBOTYPEL(2,HeroX(),HeroY()+15));
		g2 = g2 || isGrassType(COMBOTYPEL(1,HeroX()+15,HeroY()+15)) || isGrassType(COMBOTYPEL(2,HeroX()+15,HeroY()+15));
	}
	return !(g1&&g2);
}

void dTallGrass::draw(BITMAP *dest)
{
	if(HeroGetDontDraw())
		return;
		
	int32_t t=0;
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].newtile*4;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwTallGrass].newtile*4;
		cs=wpnsbuf[iwTallGrass].csets&15;
		
	}
	
	flip=0;
	x=HeroX();
	y=HeroY()+10;
	
//  if (BSZ)
	if(zinit.heroAnimationStyle==las_bszelda)
	{
		tile=t+(anim_3_4(HeroLStep(),7)*2);
	}
	else
	{
		tile=t+((HeroLStep()>=6)?2:0);
	}
	
	decoration::draw8(dest);
	x+=8;
	++tile;
	decoration::draw8(dest);
}

dRipples::dRipples(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
}

bool dRipples::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	clk++;
	if (get_bit(quest_rules, qr_SHALLOW_SENSITIVE))
	{
		if (HeroZ() == 0 && HeroAction() != swimming && HeroAction() != sideswimming && HeroAction() != sideswimhit && HeroAction() != sideswimattacking && HeroAction() != isdiving && HeroAction() != drowning)
		{
			/*
			return !((FFORCOMBOTYPE(HeroX()+11,HeroY()+15)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+11,HeroY()+15)))
			&& (FFORCOMBOTYPE(HeroX()+4,HeroY()+15)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+4,HeroY()+15)))
			&& (FFORCOMBOTYPE(HeroX()+11,HeroY()+9)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+11,HeroY()+9)))
			&& (FFORCOMBOTYPE(HeroX()+4,HeroY()+9)==cSHALLOWWATER || iswater_type(FFORCOMBOTYPE(HeroX()+4,HeroY()+9))));
			*/
			
			return !(iswaterex(FFORCOMBO(HeroX()+11,HeroY()+15), currmap, currscr, -1, HeroX()+11,HeroY()+15, false, false, true, true)
			&& iswaterex(FFORCOMBO(HeroX()+4,HeroY()+15), currmap, currscr, -1, HeroX()+4,HeroY()+15, false, false, true, true)
			&& iswaterex(FFORCOMBO(HeroX()+11,HeroY()+9), currmap, currscr, -1, HeroX()+11,HeroY()+9, false, false, true, true)
			&& iswaterex(FFORCOMBO(HeroX()+4,HeroY()+9), currmap, currscr, -1, HeroX()+4,HeroY()+9, false, false, true, true));
		}
		return true;
	}
	else
	{
		return ((COMBOTYPE(HeroX(),HeroY()+15)!=cSHALLOWWATER)||
			(COMBOTYPE(HeroX()+15,HeroY()+15)!=cSHALLOWWATER) || HeroZ() != 0);
	}
}

void dRipples::draw(BITMAP *dest)
{
	if(HeroGetDontDraw())
		return;
	
	int32_t t=0;
	
	if ( the_deco_sprite )
	{
		t=wpnsbuf[the_deco_sprite].newtile*4;
		cs=wpnsbuf[the_deco_sprite].csets&15;
		
	}
	else
	{
		t=wpnsbuf[iwRipples].newtile*4;
		cs=wpnsbuf[iwRipples].csets&15;
		
	}
	
	flip=0;
	x=HeroX();
	y=HeroY()+10;
	tile=t+(((clk/8)%3)*2);
	decoration::draw8(dest);
	x+=8;
	++tile;
	decoration::draw8(dest);
}

dHover::dHover(zfix X,zfix Y,int32_t Id,int32_t Clk, int32_t wpnSpr) : decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
	wpnid = itemsbuf[current_item_id(itype_hoverboots)].wpn;
}

void dHover::draw(BITMAP *dest)
{
	int32_t t=wpnsbuf[wpnid].newtile*4;
	cs=wpnsbuf[wpnid].csets&15;
	flip=0;
	x=HeroX();
	y=HeroY()+10-HeroZ();
	tile=t+(((clk/8)%3)*2);
	decoration::draw8(dest);
	x+=8;
	++tile;
	decoration::draw8(dest);
}

bool dHover::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	clk++;
	return HeroHoverClk()<=0;
}

dNayrusLoveShield::dNayrusLoveShield(zfix X,zfix Y,int32_t Id,int32_t Clk) : decoration(X,Y,Id,Clk)
{
	id=Id;
	clk=Clk;
}

bool dNayrusLoveShield::animate(int32_t index)
{
	index=index;  //this is here to bypass compiler warnings about unused arguments
	clk++;
	return HeroNayrusLoveShieldClk()<=0;
}

void dNayrusLoveShield::realdraw(BITMAP *dest, int32_t draw_what)
{
	if(misc!=draw_what)
	{
		return;
	}
	
	int32_t fb=(misc==0?
	        (itemsbuf[current_item_id(itype_nayruslove)].wpn5 ?
	         itemsbuf[current_item_id(itype_nayruslove)].wpn5 : (byte) iwNayrusLoveShieldFront) :
	            (itemsbuf[current_item_id(itype_nayruslove)].wpn10 ?
	             itemsbuf[current_item_id(itype_nayruslove)].wpn10 : (byte) iwNayrusLoveShieldBack));
	int32_t t=wpnsbuf[fb].newtile;
	int32_t fr=wpnsbuf[fb].frames;
	int32_t spd=wpnsbuf[fb].speed;
	cs=wpnsbuf[fb].csets&15;
	flip=0;
	bool flickering = (itemsbuf[current_item_id(itype_nayruslove)].flags & ITEM_FLAG4) != 0;
	bool translucent = (itemsbuf[current_item_id(itype_nayruslove)].flags & ITEM_FLAG3) != 0;
	
	if(((HeroNayrusLoveShieldClk()&0x20)||(HeroNayrusLoveShieldClk()&0xF00))&&(!flickering ||((misc==1)?(frame&1):(!(frame&1)))))
	{
		drawstyle=translucent?1:0;
		x=HeroX()-8;
		y=HeroY()-8-HeroZ();
		tile=t;
		
		if(fr>0&&spd>0)
		{
			tile+=((clk/spd)%fr);
		}
		
		decoration::draw(dest);
		x+=16;
		tile+=fr;
		decoration::draw(dest);
		x-=16;
		y+=16;
		tile+=fr;
		decoration::draw(dest);
		x+=16;
		tile+=fr;
		decoration::draw(dest);
	}
}

void dNayrusLoveShield::draw(BITMAP *dest)
{
	realdraw(dest,0);
}

void dNayrusLoveShield::draw2(BITMAP *dest)
{
	realdraw(dest,1);
}


/*** end of sprite.cc ***/

