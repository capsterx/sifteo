/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>

#include "assets.gen.h"
using namespace Sifteo;

// METADATA

Metadata M = Metadata()
    .title("What Am I going to do?")
    .package("com.sifteo.capsterx.test", "0.1")
    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);

AssetSlot gMainSlot = AssetSlot::allocate()
    .bootstrap(BootstrapAssets);

// GLOBALS
  
// one video-buffer per cube
Array<VideoBuffer, CUBE_ALLOCATION> vbuf; 
// new cubes as a result of paint()
CubeSet newCubes; 
// lost cubes as a result of paint()
CubeSet lostCubes; 
// reconnected (lost->new) cubes as a result of paint()
CubeSet reconnectedCubes; 
// dirty cubes as a result of paint()
CubeSet dirtyCubes;
// cubes showing the active scene
CubeSet activeCubes;

// global asset loader (each cube will have symmetric assets)
AssetLoader loader;
// global asset configuration (will just hold the bootstrap group)
AssetConfiguration<1> config; 

class Connection
{
  class Our_Cubes
  {
  public:
    CubeID cid;
  
    Our_Cubes(CubeID cid)
      : cid(cid)
    {
    }

    Our_Cubes()
    {
    }
  
    int barSpriteCount() 
    {
        // how many bars are showing on this cube?
        ASSERT(activeCubes.test(cid));
        int result = 0;
        for(int i=0; i<4; ++i) {
            if (!vbuf[cid].sprites[i].isHidden()) {
                result++;
            }
        }
        return result;
    }
  
    Int2 getRestPosition(Side s) {
      // Look up the top-left pixel of the bar for the given side.
      // We use a switch so that the compiler can optimize this
      // however if feels is best.
      switch(s) {
        case TOP: return vec(64 - Bars[0].pixelWidth()/2,0);
        case LEFT: return vec(0, 64 - Bars[1].pixelHeight()/2);
        case BOTTOM: return vec(64 - Bars[2].pixelWidth()/2, 128-Bars[2].pixelHeight());
        case RIGHT: return vec(128-Bars[3].pixelWidth(), 64 - Bars[3].pixelHeight()/2);
        default: return vec(0,0);
      }
    }
    
    bool showSideBar(Side s) {
        // if cid is not showing a bar on side s, show it and check if the
        // smiley should wake up
        ASSERT(activeCubes.test(cid));
        if (vbuf[cid].sprites[s].isHidden()) {
            vbuf[cid].sprites[s].setImage(Bars[s]);
            vbuf[cid].sprites[s].move(getRestPosition(s));
            if (barSpriteCount() == 1) {
                vbuf[cid].bg0.image(vec(0,0), Backgrounds, 1);
            }
            return true;
        } else {
            return false;
        }
    }
    
    bool hideSideBar(Side s) {
        // if cid is showing a bar on side s, hide it and check if the
        // smiley should go to sleep
        ASSERT(activeCubes.test(cid));
        if (!vbuf[cid].sprites[s].isHidden()) {
            vbuf[cid].sprites[s].hide();
            if (barSpriteCount() == 0) {
                vbuf[cid].bg0.image(vec(0,0), Backgrounds, 0);
            }
            return true;
        } else {
            return false;
        }
    }
  
    void activateCube() {
      // mark cube as active and render its canvas
      activeCubes.mark(cid);
      vbuf[cid].initMode(BG0_SPR_BG1);
      vbuf[cid].bg0.image(vec(0,0), Backgrounds, 0);
      auto neighbors = vbuf[cid].physicalNeighbors();
      for(int side=0; side<4; ++side) {
        if (neighbors.hasNeighborAt(Side(side))) {
          showSideBar(Side(side));
        } else {
          hideSideBar(Side(side));
        }
      }
    }
  };

  Our_Cubes cubes[CUBE_ALLOCATION];
  
public:
  Connection()
  {
    // initialize asset configuration and loader
    config.append(gMainSlot, BootstrapAssets);
    loader.init();

    // subscribe to events
    Events::cubeConnect.set(&Connection::onCubeConnect, this);
    Events::cubeDisconnect.set(&Connection::onCubeDisconnect, this);
    Events::cubeRefresh.set(&Connection::onCubeRefresh, this);

    Events::neighborAdd.set(&Connection::onNeighborAdd, this);
    Events::neighborRemove.set(&Connection::onNeighborRemove, this);
    
    // initialize cubes
    AudioTracker::setVolume(0.2f * AudioChannel::MAX_VOLUME);
    AudioTracker::play(Music);
    vbuf.setCount(CUBE_ALLOCATION);
    
    for(CubeID cid : CubeSet::connected()) {
      cubes[cid].cid = cid;
      vbuf[cid].attach(cid);
      cubes[cid].activateCube();
    }
  }
  
  void playSfx(const AssetAudio& sfx) {
      int i=0;
      AudioChannel(i).play(sfx);
      i = 1 - i;
  }
  
  void paintWrapper() {
      // clear the palette
      newCubes.clear();
      lostCubes.clear();
      reconnectedCubes.clear();
      dirtyCubes.clear();
  
      // fire events
      System::paint();
  
      // dynamically load assets just-in-time
      if (!(newCubes | reconnectedCubes).empty()) {
          AudioTracker::pause();
          playSfx(SfxConnect);
          loader.start(config);
          while(!loader.isComplete()) {
              for(CubeID cid : (newCubes | reconnectedCubes)) {
                  vbuf[cid].bg0rom.hBargraph(
                      vec(0, 4), loader.cubeProgress(cid, 128), BG0ROMDrawable::ORANGE, 8
                  );
              }
              // fire events while we wait
              System::paint();
          }
          loader.finish();
          AudioTracker::resume();
      }
  
      // repaint cubes
      for(CubeID cid : dirtyCubes) {
          cubes[cid].activateCube();
      }
      
      // also, handle lost cubes, if you so desire :)
  }
  
  void onCubeConnect(unsigned cid) {
      // this cube is either new or reconnected
      if (lostCubes.test(cid)) {
          // this is a reconnected cube since it was already lost this paint()
          lostCubes.clear(cid);
          reconnectedCubes.mark(cid);
      } else {
          // this is a brand-spanking new cube
          newCubes.mark(cid);
      }
      // begin showing some loading art (have to use BG0ROM since we don't have assets)
      dirtyCubes.mark(cid);
      auto & g = vbuf[cid];
      g.attach(cid);
      g.initMode(BG0_ROM);
      g.bg0rom.fill(vec(0,0), vec(16,16), BG0ROMDrawable::SOLID_BG);
      g.bg0rom.text(vec(1,1), "Hold on!", BG0ROMDrawable::BLUE);
      g.bg0rom.text(vec(1,14), "Adding Cube...", BG0ROMDrawable::BLUE);
  }
  
  void onCubeDisconnect(unsigned cid) {
      // mark as lost and clear from other cube sets
      lostCubes.mark(cid);
      newCubes.clear(cid);
      reconnectedCubes.clear(cid);
      dirtyCubes.clear(cid);
      activeCubes.clear(cid);
  }
  
  void onCubeRefresh(unsigned cid) {
      // mark this cube for a future repaint
      dirtyCubes.mark(cid);
  }
  
  bool isActive(NeighborID nid) {
      // Does this nid indicate an active cube?
      return nid.isCube() && activeCubes.test(nid);
  }
  
  void onNeighborAdd(unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
      // update art on active cubes (not loading cubes or base)
      bool sfx = false;
      if (isActive(cube0)) { sfx |= cubes[cube0].showSideBar(Side(side0)); }
      if (isActive(cube1)) { sfx |= cubes[cube1].showSideBar(Side(side1)); }
      if (sfx) { playSfx(SfxAttach); }
  }
  
  void onNeighborRemove(unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
      // update art on active cubes (not loading cubes or base)
      bool sfx = false;
      if (isActive(cube0)) { sfx |= cubes[cube0].hideSideBar(Side(side0)); }
      if (isActive(cube1)) { sfx |= cubes[cube1].hideSideBar(Side(side1)); }
      if (sfx) { playSfx(SfxDetach); }
  }
};

Connection conn;

void main() {
    // run loop
    for(;;) {
      conn.paintWrapper();
    }
}
