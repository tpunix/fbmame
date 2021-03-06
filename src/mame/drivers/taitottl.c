/***************************************************************************

 Taito Discrete Hardware Games


 Game Name(s)                                               Part #'s       Data      PROM/ROM Chip Numbers
 ---------------------------------------------------------+--------------+---------+---------------------------------------
 Astro Race (11/1973)                                                      UNKNOWN
 Attack (09/1976)                                                          UNKNOWN
 Attack UFO (08/1974)                                                      UNKNOWN
 Avenger (??/1976) (clone of Electra's Avenger)                            UNKNOWN
 Ball Park/TT Ball Park (??/1974)                                          UNKNOWN
 Basketball (04/1974)                                                      UNKNOWN
 Bombs Away (??/1977)                                                      UNKNOWN
 Cisco/Fisco 400 (04/1977)                                                 UNKNOWN
 Clean Sweep (??/1976) (clone of Ramtek's Clean Sweep)                     UNKNOWN
 Clean Sweep II (??/1976) (clone of Ramtek's Clean Sweep)                  UNKNOWN
 Crashing Race (06/1976)                                                   UNKNOWN
 Cross Fire (08/1977) (aka Bazooka)                                        UNKNOWN
 Davis Cup (12/1973)                                                       UNKNOWN
 Dead Heat (??/1975)                                                       UNKNOWN
 Elepong (07/1973)                                                         UNKNOWN
 Flying Fortress (??/1977)                                                 UNKNOWN
 Flying Fortress II (06/1977)                                             UNKNOWN
 Gunman (10/1977)                                                          YES        8 x 32bytes (or 11? )
 Interceptor (03/1976)                                                     UNKNOWN
 Missile-X (??/1977)                                                       YES        10 - (5 x 512bytes, 5x32bytes)
 Pro Hockey (11/1973)                                                      UNKNOWN
 Road Champion (04/1977)                                                   UNKNOWN
 Road Champion S (06/1977)                                                 UNKNOWN
 Soccer (11/1973)                                                          UNKNOWN
 Soccer DX (??/1977)                                                       UNKNOWN
 Speed Race (11/1974)                                                      UNKNOWN
 Speed Race CL-5 (10/1978)                                                 UNKNOWN
 Speed Race Color (10/1978)                                                UNKNOWN
 Speed Race DX (08/1975)                                                   UNKNOWN
 Speed Race Twin (04/1976)                                                 UNKNOWN
 Speed Race GP-5 (??/1980)                                                 UNKNOWN
 Super Block (02/1978)                                                     UNKNOWN
 Super Speed Race (12/1977)                                                UNKNOWN
 Super Speed Race V (07/1978)                                              UNKNOWN
 Super Speed Race GP-V (??/1980)                                           UNKNOWN
 Table Football (??/1977)                                                  UNKNOWN
 Tahitian (??/1975)                                                        UNKNOWN
 Tennis (??/1977)                                                          UNKNOWN
 T. T. Block (08/1977)                                                     YES        1 x 2048bytes
 T. T. Block C (05/1978)                                                   UNKNOWN
 T. T. Block CU (08/1978)                                                  UNKNOWN
 T. T. Speed Race (??/1978)                                                UNKNOWN
 Wall Block (08/1978)                                                      UNKNOWN
 Wall Break (01/1977)                                                      UNKNOWN
 Western Gun (09/1975)                                                     UNKNOWN
 Zun Zun Block (04/1979)                                                   YES        3 - (2 x 512bytes, 1 x 32bytes)

***************************************************************************/


#include "emu.h"

#include "machine/netlist.h"
#include "netlist/devices/net_lib.h"
#include "video/fixfreq.h"
#include "astring.h"

// copied by Pong, not accurate for this driver!
// start
#define MASTER_CLOCK    7159000
#define V_TOTAL         (0x105+1)       // 262
#define H_TOTAL         (0x1C6+1)       // 454

#define HBSTART                 (H_TOTAL)
#define HBEND                   (80)
#define VBSTART                 (V_TOTAL)
#define VBEND                   (16)

#define HRES_MULT                   (1)
// end


class taitottl_state : public driver_device
{
public:
	taitottl_state(const machine_config &mconfig, device_type type, const char *tag)
	: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_video(*this, "fixfreq")
	{
	}

	// devices
	required_device<netlist_mame_device_t> m_maincpu;
	required_device<fixedfreq_device> m_video;

protected:

	// driver_device overrides
	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();

private:

};


static NETLIST_START(taitottl)
	SOLVER(Solver, 48000)
//  PARAM(Solver.FREQ, 48000)
	PARAM(Solver.ACCURACY, 1e-4) // works and is sufficient

	// schematics
	//...

	//  NETDEV_ANALOG_CALLBACK(sound_cb, sound, exidyttl_state, sound_cb, "")
	//  NETDEV_ANALOG_CALLBACK(video_cb, videomix, fixedfreq_device, update_vid, "fixfreq")
NETLIST_END()



void taitottl_state::machine_start()
{
}

void taitottl_state::machine_reset()
{
}


void taitottl_state::video_start()
{
}

static MACHINE_CONFIG_START( taitottl, taitottl_state )

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", NETLIST_CPU, NETLIST_CLOCK)
	MCFG_NETLIST_SETUP(taitottl)

	/* video hardware */
	MCFG_FIXFREQ_ADD("fixfreq", "screen")
	MCFG_FIXFREQ_MONITOR_CLOCK(MASTER_CLOCK)
	MCFG_FIXFREQ_HORZ_PARAMS(H_TOTAL-67,H_TOTAL-40,H_TOTAL-8,H_TOTAL)
	MCFG_FIXFREQ_VERT_PARAMS(V_TOTAL-22,V_TOTAL-19,V_TOTAL-12,V_TOTAL)
	MCFG_FIXFREQ_FIELDCOUNT(1)
	MCFG_FIXFREQ_SYNC_THRESHOLD(0.30)
MACHINE_CONFIG_END


/***************************************************************************

 Game driver(s)

 ***************************************************************************/

/*

 label  loc. Part #
 ==========================================================
 CR11   N5  74s287      yes, it says CRxx not GNxx
 GN10   H5  5623
 GN09   N6  soldered in
 GN08   J7  IM5200CJG
 GN07   L9  soldered in
 GN06   H6  7621
 GN05   G9  74s287
 GN04   F9  7643
 GN03   D9  7621
 GN02   C9  74s287
 GN01   B9  7643

*/

ROM_START( gunman )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x0800, "roms", ROMREGION_ERASE00 )
	ROM_LOAD( "cr11.n5",     0x0000, 0x0200, CRC(8e2ec052) SHA1(b849080c9ed325a478739fbdf315c8a6c2bb7bd7) )
	ROM_LOAD( "gn10.h5",     0x0000, 0x0200, CRC(a976c4c7) SHA1(f2fb56ccec99ceb19d607bee3b4cca31d6db58fd) )
	ROM_LOAD( "gn06.h6",     0x0000, 0x0400, CRC(17376de6) SHA1(4ef660da9d37024a13925af12a7e102a49c67ff6) )
	ROM_LOAD( "gn05.g9",     0x0000, 0x0200, CRC(f3d7f6cb) SHA1(7abf445cc814dc242ebf2393e9c1335ba4aedb90) )
	ROM_LOAD( "gn04.f9",     0x0000, 0x0800, CRC(dec942db) SHA1(165d8594e07837fd4a87895f0a350d5ed8112ead) )
	ROM_LOAD( "gn03.d9",     0x0000, 0x0400, CRC(b0e6e473) SHA1(9665608b7ec1dfb39e4e1df74adbfc773e9a6aa7) )
	ROM_LOAD( "gn02.c9",     0x0000, 0x0200, CRC(897ae7b8) SHA1(2e91bc8336f8620faedd5329ce7d9734f19a2cfa) )
	ROM_LOAD( "gn01.b9",     0x0000, 0x0800, CRC(69474a3a) SHA1(7396f1be991a0e6207eaa79e0206a5286a8d615d) )
ROM_END


ROM_START( bazooka )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x0840, "roms", ROMREGION_ERASE00 )
	ROM_LOAD( "bd2.k1",      0x0000, 0x0200, CRC(c9e9ed15) SHA1(624bbc10942a386040aef161b96d64021a842c9f) ) // 6341-1 - gfx: tank, truck, jeep motorcycle
	ROM_LOAD( "bd2.k4",      0x0200, 0x0200, CRC(c5a74df9) SHA1(2846a039e9bf372f3aa0b88ed89f9029eb7f797c) ) // 6341-1 - gfx: ambulance, stretcher, explosion
	ROM_LOAD( "bd1.d2",      0x0400, 0x0200, CRC(4fc10886) SHA1(b1c6f890994ba2182a4e7fc17582d6797dbd6ce9) ) // 6341-1 or 82s115
	ROM_LOAD( "bd1.e2",      0x0600, 0x0200, CRC(00179936) SHA1(e5417b8d3814dafe1278179b307a1b563a378cbe) ) // 6341-1 or 82s115
	ROM_LOAD( "bd2.e6",      0x0800, 0x0020, CRC(14b84564) SHA1(69cdd14e23094678c4b280f60cec963609181b00) ) // 82123
	ROM_LOAD( "bd2.e7",      0x0820, 0x0020, CRC(1bfb073f) SHA1(f6b26dcece71b2cf2ed4a537434edbe31cb10399) ) // 82123
ROM_END


ROM_START( bazookabr )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x0840, "roms", ROMREGION_ERASE00 )
	ROM_LOAD( "1",           0x0000, 0x0200, CRC(edc34cb0) SHA1(f76a81833b015784e55b33189e9058cd24922f9b) )
	ROM_LOAD( "2",           0x0200, 0x0200, CRC(3e78e4c2) SHA1(814509eb773bfa87f1df933214f079e7dd2a8fa2) )
	ROM_LOAD( "3",           0x0400, 0x0200, CRC(4fc10886) SHA1(b1c6f890994ba2182a4e7fc17582d6797dbd6ce9) )
	ROM_LOAD( "4",           0x0600, 0x0200, CRC(00179936) SHA1(e5417b8d3814dafe1278179b307a1b563a378cbe) )
	ROM_LOAD( "bd2.e6",      0x0800, 0x0020, BAD_DUMP CRC(14b84564) SHA1(69cdd14e23094678c4b280f60cec963609181b00) ) // not dumped, taken from PSE set
	ROM_LOAD( "bd2.e7",      0x0820, 0x0020, BAD_DUMP CRC(1bfb073f) SHA1(f6b26dcece71b2cf2ed4a537434edbe31cb10399) ) // not dumped, taken from PSE set
ROM_END


/*

 Missile-X
 14.314 mhz XTAL

 label  loc. Part #     Use?
 ==========================================================
 CR11   L4  74s287      numerics (yes, it says CRxx not GNxx)
 MS09   F6  74s287      ground explosion
 MS08   C2  74s287      400pt jeep graphics
 MS07   N8  74s287      player missile
 MS06   M8  74s287      missile animated graphics
 MS05   11F IM5610      200pt tank R->L graphic
 MS05   11E IM5610      200pt tank L->R graphic
 MS04   N7  IM5610      player missile trajectory pattern
 MS03   F3  IM5610      anti-missile ack-ack graphics
 MS02   L12 IM5610      missile left/right position
 MS01   D8  IM5610      100pt tanks graphics

*/

ROM_START( missilex )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x0800, "roms", ROMREGION_ERASE00 )
	ROM_LOAD( "cr11.4l",      0x0000, 0x0200, CRC(3d10a407) SHA1(1a85581c34d7e6766eaebbcc9895ed0ab2f94387) )
	ROM_LOAD( "ms09.6f",      0x0000, 0x0200, CRC(80787642) SHA1(9de2419f0ba16f9c2b06e417c1ebba441fdae053) )
	ROM_LOAD( "ms08.2c",      0x0000, 0x0200, CRC(dd785590) SHA1(7ba6b6f6091595852d6feaef5a029b2aca684440) )
	ROM_LOAD( "ms07.8n",      0x0000, 0x0200, CRC(e278d03a) SHA1(c40975e5807936fed40cda4a6881f6aef0e7f350) )
	ROM_LOAD( "ms06.8m",      0x0000, 0x0200, CRC(fe6c9192) SHA1(d110e010cf685ee18479ca7f890fa9da2fa71603) )
	ROM_LOAD( "ms05.11f",     0x0000, 0x0020, CRC(845fe0cc) SHA1(ce8db615c1f7be242fc2ee25c1ef75e8608a771a) )
	ROM_LOAD( "ms05.11e",     0x0000, 0x0020, CRC(845fe0cc) SHA1(ce8db615c1f7be242fc2ee25c1ef75e8608a771a) )
	ROM_LOAD( "ms04.7n",      0x0000, 0x0020, CRC(34d0bee4) SHA1(41848def6aeb128ec985c158f3ed01c5b20bdcf6) )
	ROM_LOAD( "ms03.3f",      0x0000, 0x0020, CRC(d139f5fa) SHA1(29c05143d05553c2cb2831f9624f307f59436850) )
	ROM_LOAD( "ms02.12l",     0x0000, 0x0020, CRC(157b7e68) SHA1(d1a98267af1562e6126faaf0850906224f8a608d) )
	ROM_LOAD( "ms01.8d",      0x0000, 0x0020, CRC(e89e76c3) SHA1(1149b5d1f93baa8aecd54a618083cc13b63a894d) )
ROM_END


ROM_START( ttblock )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x0800, "roms", ROMREGION_ERASE00 )
	ROM_LOAD( "tl01.4a",     0x0000, 0x0800, CRC(65b730f7) SHA1(f82931c9a128021c97d1d41b5eac05df55dd5994) ) // MMI 6353
ROM_END


ROM_START( zzblock )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )

	ROM_REGION( 0x0400, "roms", ROMREGION_ERASE00 )
	ROM_LOAD( "tz01.1e",     0x0000, 0x0200, CRC(bd1e59e0) SHA1(038ad0b1b629c6c04f4d885fce6fe1c1f72b33e5) )
	ROM_LOAD( "tz02.1d",     0x0000, 0x0200, CRC(befc8ad3) SHA1(80ce2a9978d22d45ccbe055c7dafba5c0ad6cf3c) )
	ROM_LOAD( "tz03.6f",     0x0000, 0x0020, CRC(1ba385b1) SHA1(b70c356174a1748723c0116ae8dd74b25e1fe6b4) )
ROM_END


GAME( 1977, gunman,    0,       taitottl, 0, driver_device,  0, ROT0, "Taito", "Gunman [TTL]", GAME_IS_SKELETON )
GAME( 1977, bazooka,   0,       taitottl, 0, driver_device,  0, ROT0, "Project Support Engineering", "Bazooka [TTL]", GAME_IS_SKELETON ) // clone of Taito Cross Fire - or is Cross Fire a clone of PSE Bazooka?
GAME( 1977, bazookabr, bazooka, taitottl, 0, driver_device,  0, ROT0, "Taito do Brasil", "Bazooka (Brazil) [TTL]", GAME_IS_SKELETON )
GAME( 1977, missilex,  0,       taitottl, 0, driver_device,  0, ROT0, "Taito", "Missile-X [TTL]", GAME_IS_SKELETON )
GAME( 1977, ttblock,   0,       taitottl, 0, driver_device,  0, ROT0, "Taito", "T.T. Block [TTL]", GAME_IS_SKELETON )
GAME( 1979, zzblock,   0,       taitottl, 0, driver_device,  0, ROT0, "Taito", "Zun Zun Block [TTL]", GAME_IS_SKELETON )
