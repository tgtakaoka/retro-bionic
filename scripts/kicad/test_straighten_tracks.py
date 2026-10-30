"""Tests for straighten_tracks.py on small synthetic boards.

    cd scripts/kicad && python3 -m unittest test_straighten_tracks
"""
import unittest

import pcbnew

from straighten_tracks import P, Straightener, key, on_axis

V = pcbnew.VECTOR2I_MM


class Board:
    def __init__(self):
        self.b = pcbnew.BOARD()
        self.b.SetCopperLayerCount(4)
        self.net = pcbnew.NETINFO_ITEM(self.b, 'N')
        self.b.Add(self.net)

    def pad(self, x, y, w=1.4, h=0.3):
        fp = pcbnew.FOOTPRINT(self.b)
        p = pcbnew.PAD(fp)
        p.SetAttribute(pcbnew.PAD_ATTRIB_SMD)
        p.SetShape(pcbnew.PAD_SHAPE_RECT)
        p.SetLayerSet(p.SMDMask())
        p.SetSize(V(w, h))
        fp.Add(p)
        fp.SetPosition(V(x, y))
        p.SetNet(self.net)
        self.b.Add(fp)

    def via(self, x, y):
        v = pcbnew.PCB_VIA(self.b)
        v.SetPosition(V(x, y))
        v.SetWidth(pcbnew.FromMM(0.45))
        v.SetDrill(pcbnew.FromMM(0.2))
        v.SetNet(self.net)
        self.b.Add(v)

    def track(self, *pts, layer=pcbnew.F_Cu):
        for a, c in zip(pts, pts[1:]):
            t = pcbnew.PCB_TRACK(self.b)
            t.SetStart(V(*a))
            t.SetEnd(V(*c))
            t.SetWidth(pcbnew.FromMM(0.15))
            t.SetLayer(layer)
            t.SetNet(self.net)
            self.b.Add(t)

    def run(self):
        """one pass of the three steps, as main() does without DRC"""
        Straightener(self.b).snap(set())
        Straightener(self.b).joints(set())
        st = Straightener(self.b)
        res = st.straighten()
        st.drop_idle()
        return res

    def tracks(self, layer=pcbnew.F_Cu):
        return sorted(tuple(sorted((key(P(t.GetStart())), key(P(t.GetEnd()))))) for t in self.b.GetTracks()
                      if t.Type() == pcbnew.PCB_TRACE_T and t.GetLayer() == layer)

    def vias(self):
        return [key(P(v.GetPosition())) for v in self.b.GetTracks() if v.Type() == pcbnew.PCB_VIA_T]


class StraightenTest(unittest.TestCase):
    def test_straight_run_untouched(self):
        b = Board()
        b.pad(0, 0)
        b.pad(6, 0)
        b.track((0, 0), (3, 0), (4, 1))
        b.track((4, 1), (5, 0), (6, 0))
        rebuilt, left = b.run()
        self.assertEqual((rebuilt, left), ({}, []))

    def test_micro_jog_dropped(self):
        b = Board()
        b.pad(0, 0)
        b.pad(6, 0)
        b.track((0, 0), (3, 0), (3.02, 0.02), (6, 0.02))
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (6, 0))])

    def test_pad_end_moves_onto_centre_line(self):
        b = Board()
        b.pad(0, 0)
        b.via(3, 0)
        b.track((0, 0.03), (3, 0.03), (3, 0))
        b.track((3, 0), (3, 2), layer=pcbnew.B_Cu)
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (3, 0))])

    def test_pad_end_stretched_to_centre(self):
        # a run along the pad, stopping at its edge
        b = Board()
        b.pad(0, 0)
        b.pad(6, 0)
        b.track((0.7, 0), (5.3, 0))
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (6, 0))])

    def test_pad_end_led_to_centre(self):
        # a run across the pad, touching its edge: a leg along the pad takes it to the centre
        b = Board()
        b.pad(0, 0)
        b.pad(0.6, 3, 0.3, 1.4)
        b.track((0.6, 0), (0.6, 3))
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (0.6, 0)), ((0.6, 0), (0.6, 3))])
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (0.6, 0)), ((0.6, 0), (0.6, 3))])

    def test_near_centre_end_moves_the_run(self):
        # an end 25 microns off centre along the pad: the run's lengths take it up, no leg
        b = Board()
        b.pad(0, 0)
        b.pad(2.025, 4, 0.3, 1.4)
        b.track((0.025, 0), (0.025, 2), (2.025, 4))
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (0, 1.975)), ((0, 1.975), (2.025, 4))])

    def test_pad_leg_kept_across_a_micron_step(self):
        # A5 on the H8/510: the leg onto the centre misses the run by a micron, so the run
        # (left as it was) still reaches the centre through it
        b = Board()
        b.pad(0, 0, 1.95, 0.5)
        b.pad(3, 0.3, 0.3, 1.4)
        b.track((0, 0), (0.7, 0))
        b.track((0.7009, 0.0009), (2.7009, 0.0009), (3, 0.3))
        # as when DRC turned down the run's rebuild
        Straightener(b.b).drop_idle()
        self.assertIn(((0, 0), (0.7, 0)), b.tracks())

    def test_pad_end_never_slides_across_pad(self):
        # the via cannot slide across its own horizontal track, so the run stays
        b = Board()
        b.pad(0, 0)
        b.via(3, 0.04)
        b.track((0, 0), (3, 0.04))
        b.track((3, 0.04), (6, 0.04), layer=pcbnew.B_Cu)
        rebuilt, left = b.run()
        self.assertEqual(rebuilt, {})
        self.assertEqual(len(left), 1)
        self.assertEqual(b.tracks(), [((0, 0), (3, 0.04))])

    def test_via_slides_along_its_other_track(self):
        b = Board()
        b.pad(0, 0)
        b.via(1.75, -0.05)
        b.track((0, 0), (1.75, -0.05))
        b.track((1.75, -0.05), (3.35, -1.65), layer=pcbnew.In1_Cu)
        b.run()
        (via,) = b.vias()
        self.assertAlmostEqual(via[1], 0, places=3)
        (top,) = b.tracks()
        self.assertEqual(top[1], via)
        self.assertEqual(top[0][1], 0)
        (inner,) = b.tracks(pcbnew.In1_Cu)
        self.assertEqual(inner[0], via)
        self.assertTrue(on_axis(*inner))

    def test_short_jog_into_via_dropped(self):
        # U1 pin 79 on mc9s12x: a 0.07 mm 45-degree jog into the via; the via moves to the corner
        b = Board()
        b.pad(124.3625, 96.75, 1.475, 0.3)
        b.via(122.6, 96.8)
        b.track((124.3625, 96.75), (122.65, 96.75), (122.6, 96.8))
        b.track((122.6, 96.8), (121.0, 98.4), layer=pcbnew.In1_Cu)
        b.run()
        self.assertEqual(b.vias(), [(122.65, 96.75)])
        self.assertEqual(b.tracks(), [((122.65, 96.75), (124.3625, 96.75))])
        self.assertEqual(b.tracks(pcbnew.In1_Cu), [((121.0, 98.4), (122.65, 96.75))])

    def test_jog_into_via_kept(self):
        # a 45-degree jog of 0.14 mm stays: the via's other track ends where it cannot follow
        b = Board()
        b.pad(124.3625, 96.75, 1.475, 0.3)
        b.via(122.6, 96.85)
        b.track((124.3625, 96.75), (122.7, 96.75), (122.6, 96.85))
        b.track((122.6, 96.85), (121.0, 98.45), layer=pcbnew.In1_Cu)
        before = b.tracks()
        b.run()
        self.assertEqual(b.tracks(), before)
        self.assertEqual(b.vias(), [(122.6, 96.85)])

    def test_end_short_of_via_snapped(self):
        b = Board()
        b.pad(0, 0)
        b.via(3, 0)
        b.track((0, 0), (2.997, 0.004))
        b.track((3, 0), (3, 2), layer=pcbnew.B_Cu)
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (3, 0))])

    def test_shared_end_snapped_together(self):
        # a 14 micron 45-degree stub into the via: the run it hangs off follows it
        b = Board()
        b.pad(0, 0, 0.3, 1.4)
        b.via(0.01, 5.01)
        b.track((0, 0), (0, 5), (0.01, 5.01))
        b.track((0.01, 5.01), (0.01, 8), layer=pcbnew.B_Cu)
        b.run()
        (top,) = b.tracks()
        self.assertIn(top[1], b.vias())

    def test_short_axial_stub_collapsed(self):
        # a 25 micron stub between a run and its via goes, the run ends at the via
        b = Board()
        b.pad(0, 0)
        b.via(3, 0.025)
        b.track((0, 0.025), (3, 0.025))
        b.track((0, 0), (0, 0.025))
        b.track((3, 0.025), (3, 2), layer=pcbnew.B_Cu)
        b.track((3, 0.025), (3.025, 0.025))
        b.track((3.025, 0.025), (5, 0.025), layer=pcbnew.F_Cu)
        b.run()
        self.assertTrue(all(abs(e[0] - s[0]) + abs(e[1] - s[1]) >= 0.05 for s, e in b.tracks()))

    def test_joint_moves_onto_axes(self):
        # a via a few microns off both its tracks' lines
        b = Board()
        b.pad(0, 0)
        b.pad(3, 3, 0.3, 1.4)
        b.via(3.006, 0.004)
        b.track((0, 0), (3.006, 0.004))
        b.track((3.006, 0.004), (3, 3), layer=pcbnew.B_Cu)
        b.run()
        self.assertEqual(b.vias(), [(3, 0)])
        self.assertEqual(b.tracks(), [((0, 0), (3, 0))])
        self.assertEqual(b.tracks(pcbnew.B_Cu), [((3, 0), (3, 3))])

    def test_via_shifts_across_its_runs(self):
        # D7 on mc68008fn: a via 5 microns off its pad's centre line, its other run horizontal
        b = Board()
        b.pad(6, 0)
        b.via(3, 0.005)
        b.via(-2, -1.995)
        b.track((3, 0.005), (6, 0))
        b.track((3, 0.005), (0, 0.005), (-2, -1.995), layer=pcbnew.B_Cu)
        b.track((-2, -1.995), (-2, -5), layer=pcbnew.In1_Cu)
        b.run()
        self.assertIn((3, 0), b.vias())
        self.assertEqual(b.tracks(), [((3, 0), (6, 0))])
        self.assertTrue(all(on_axis(*t) for t in b.tracks(pcbnew.B_Cu)))

    def test_via_between_stubs_moves_to_corner(self):
        # A14 on mc68008fn: 45-degree stubs on both sides of the via, then the real runs
        b = Board()
        b.pad(3, 0.0575)
        b.pad(-0.053, -5, 0.3, 1.4)
        b.via(0, 0)
        b.track((0, 0), (0.0575, 0.0575), (3, 0.0575))
        b.track((0, 0), (-0.053, -0.053), (-0.053, -5), layer=pcbnew.B_Cu)
        b.run()
        self.assertEqual(b.vias(), [(-0.053, 0.0575)])
        self.assertEqual(b.tracks(), [((-0.053, 0.0575), (3, 0.0575))])
        self.assertEqual(b.tracks(pcbnew.B_Cu), [((-0.053, -5), (-0.053, 0.0575))])

    def test_via_centred_on_pad_line(self):
        # ASEL0 at U6 on mc68000fn: the via 0.1 mm off the pad's line, a 45-degree jog into it
        b = Board()
        b.pad(0, 0, 1.778, 0.3556)
        b.pad(9.3025, 1.4, 1.0, 2.5)
        b.via(1.277, -0.1025)
        b.track((0, 0), (1.1745, 0), (1.277, -0.1025))
        b.track((1.277, -0.1025), (7.8, -0.1025), (9.3025, 1.4), layer=pcbnew.B_Cu)
        # the via's other front run: its long leg keeps to its line
        b.pad(1.0, -8.0, 0.3556, 1.778)
        b.track((1.277, -0.1025), (2.1817, -1.0072), (2.1817, -6.0), (1.0, -7.1817), (1.0, -8.0))
        b.run()
        self.assertEqual(b.vias(), [(1.277, 0)])
        self.assertIn(((2.1817, -6.0), (2.1817, -0.9047)), b.tracks())
        self.assertIn(((0, 0), (1.277, 0)), b.tracks())
        self.assertEqual(b.tracks(pcbnew.B_Cu), [((1.277, 0), (7.9025, 0)), ((7.9025, 0), (9.3025, 1.4))])

    def test_via_slides_along_pad_onto_centre(self):
        # ~RES at J1-23 on the H8/536: a via 25 microns along the pad, one leg across it
        b = Board()
        b.pad(0, 0, 2.51, 1.0)
        b.via(0.0254, -0.8636)
        b.track((0.0254, -0.8636), (0.0254, 0))
        b.track((0.0254, -0.8636), (0.0254, -4), (1.0254, -5), layer=pcbnew.B_Cu)
        b.run()
        self.assertEqual(b.vias(), [(0, -0.8636)])
        self.assertEqual(b.tracks(), [((0, -0.8636), (0, 0))])

    def test_via_far_off_pad_line_kept(self):
        # a via 0.2 mm off the pad's line is a fan-out
        b = Board()
        b.pad(0, 0, 1.778, 0.3556)
        b.pad(9.2, 1.5, 1.0, 2.5)
        b.via(1.4, -0.2)
        b.track((0, 0), (1.2, 0), (1.4, -0.2))
        b.track((1.4, -0.2), (7.5, -0.2), (9.2, 1.5), layer=pcbnew.B_Cu)
        before = b.tracks()
        b.run()
        self.assertEqual(b.vias(), [(1.4, -0.2)])
        self.assertEqual(b.tracks(), before)

    def test_via_across_pad_line_kept(self):
        # a leg across the pad's line sets the via aside on purpose
        b = Board()
        b.pad(0, 0, 1.778, 0.3556)
        b.pad(9.2, 1.5, 1.0, 2.5)
        b.via(1.5, -0.1)
        b.track((0, 0), (1.5, 0), (1.5, -0.1))
        b.track((1.5, -0.1), (7.6, -0.1), (9.2, 1.5), layer=pcbnew.B_Cu)
        b.run()
        self.assertEqual(b.vias(), [(1.5, -0.1)])

    def test_loose_end_joined(self):
        # ASEL0 on mc68000fn: a run's end stops 5 microns past the next track's end
        b = Board()
        b.pad(0, 0)
        b.pad(6, 1.4, 1.0, 2.5)
        b.track((0, 0), (4.6048, 0))
        b.track((4.6, 0), (6, 1.4))
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (4.6, 0)), ((4.6, 0), (6, 1.4))])

    def test_track_on_track_dropped(self):
        b = Board()
        b.pad(0, 0)
        b.pad(6, 0)
        b.track((0, 0), (6, 0))
        b.track((2, 0), (3, 0))
        b.run()
        self.assertEqual(b.tracks(), [((0, 0), (6, 0))])


if __name__ == '__main__':
    unittest.main()
