#!/usr/bin/env python

# This program generates XML file based on input matroska file
# The file layout should mostly correspond to EBML elements layout in the input file 
# (visible in "mkvinfo -v" output)
# SimpleBlock and Block elements have special format: <track> subelement has track number, there can be additional subelements for flags, <timecode> subelement shows _absolute_ timecode of this block (based on containing Cluster timecode) in seconds. and one or more <data> elements (for laced frames) and optionally <duration> (which corresponts to containing BlockGroup's duration, in seconds. All other times (outside [Simple]Blocks) are just unscaled numbers.
# By default some elements (which does refer positions inside the file, like Cues or SeekHeads) are stripped, "-v" option makes output everything verbatim
# "-C" option also inhibits "<Cluster>" elements (and all containing things) and intead of "SimpleBlock" or "BlockGroup" produces just <block> (timecodes are absolute and duration is included, so we can reconstruct the most things back if needed).

# Subtitles (CodecPrivate and blocks that corresponts to S_TEXT tracks) are retained as UTF-8 text (in CDATAs), other binary data is in hex.

# mkvparse "flattens" Segments and Clusters, providing them as top-level elements (to simplify resyncing), so there are additional hacks to make output XML file look more like Matroska elemet tree.

# 2012, Vitaly "_Vi" Shukela, License=MIT


import mkvparse
import sys
import re
import binascii
from os import getenv

from xml.sax import saxutils


if sys.version < '3':
    reload(sys)
    sys.setdefaultencoding('utf-8')   # hack to stop failing on Unicodish subtitles
    range=xrange
    maybe_decode=lambda x:x
else:
    maybe_decode=lambda x:x.decode("ascii")


chunklength = 64


def chunks(s, n):
    """Produce `n`-character chunks from `s`."""
    if not n:
       yield s
       return
    for start in range(0, len(s), n):
        yield s[start:start+n]


class MatroskaToText(mkvparse.MatroskaHandler):
    def __init__(self, banlist, nocluster_mode):
        self.there_was_segment=False
        self.there_was_cluster=False
        self.indent=0

        self.lb_track_id=None
        self.lb_ts=None
        self.lb_data=[]
        self.lb_duration=None
        self.banlist=banlist
        self.nocluster_mode = nocluster_mode

        self.text_tracks=frozenset([])
        self.going_to_be_textual_codec_private = False

        self.lb_duration=None
        self.lb_track_id=555
        self.lb_ts=555.5
        self.lb_data=[]
        self.lb_keyframe = False
        self.lb_invisible = False
        self.lb_discardable = False
        print("<mkv2xml>")

    def __del__(self):
        if self.there_was_cluster:
            print("</Cluster>")
        if self.there_was_segment:
            print("</Segment>")
        print("</mkv2xml>")

    def tracks_available(self):
        text_tracks=[]
        for k in self.tracks:
            t=self.tracks[k]
            if t['CodecID'][1][:6] == "S_TEXT":
                text_tracks.append(k)
        self.text_tracks=frozenset(text_tracks)

    def segment_info_available(self):
        pass;

    def frame(self, track_id, timestamp, data, more_laced_frames, duration, keyframe, invisible, discardable):
        self.lb_duration=duration
        self.lb_track_id=track_id
        self.lb_ts=timestamp
        self.lb_data.append(data)
        self.lb_keyframe = keyframe
        self.lb_invisible = invisible
        self.lb_discardable = discardable

        if self.nocluster_mode:
            print("<block>%s</block>" % self.format_block(""))            

    def format_block(self, indent_):
        newdata ="\n  "+indent_+"<track>%s</track>"%self.lb_track_id;
        newdata+="\n  "+indent_+"<timecode>%s</timecode>"%self.lb_ts;
        if self.lb_duration:
            newdata+="\n  "+indent_+"<duration>%s</duration>"%self.lb_duration;
        if self.lb_keyframe:     newdata+="\n  "+indent_+"<keyframe/>" 
        if self.lb_invisible:    newdata+="\n  "+indent_+"<invisible/>"
        if self.lb_discardable:  newdata+="\n  "+indent_+"<discardable/>"
        for data_2 in self.lb_data:
            decoded_as_text = False
            if self.lb_track_id in self.text_tracks:
                try:
                    data_2 = data_2.decode("UTF-8")
                    newdata+="\n  "+indent_+"<data encoding=\"text\"><![CDATA["
                    newdata+= data_2.replace("\x00","").replace("]]>", "]]]]><![CDATA[>")
                    newdata+="]]></data>"
                    decoded_as_text = True
                except UnicodeDecodeError:
                    sys.stderr.write("UTF-8 decoding error, leaving as binary\n")
            if not decoded_as_text:
                newdata+="\n  "+indent_+"<data>";
                for chunk in chunks(maybe_decode(binascii.hexlify(data_2)), chunklength):
                    newdata+="\n    "+indent_;
                    newdata+=chunk;
                newdata+="\n  "+indent_+"</data>";
        newdata+="\n"+indent_;
        self.lb_duration=None
        self.lb_track_id=None
        self.lb_ts=None
        self.lb_data=[]
        return newdata

    def printtree(self, list_, indent):
        indent_ = "  "*indent;
        for (name_, (type_, data_)) in list_:
            opening_tag_content = name_
            if self.there_was_cluster and \
                name_ != "Timecode" and \
                name_ != "Timestamp" and \
                name_ != "SilentTracks" and \
                name_ != "Position" and \
                name_ != "PrevSize" and \
                name_ != "SimpleBlock" and \
                name_ != "BlockGroup" and \
                name_ != "Void" and \
                name_ != "CRC-32" and \
                name_ != "SignatureSlot" and \
                name_ != "EncryptedBlock" and \
                indent == self.indent:
                # looks like our Cluster aleady ended somewhere
                indent_=""
                indent=0
                self.indent=0
                print("</Cluster>")
                self.there_was_cluster=False

            if name_ == "TrackNumber":
                self.going_to_be_textual_codec_private = data_ in self.text_tracks

            if name_ in self.banlist:
                continue;
            if type_ == mkvparse.EbmlElementType.BINARY:
                if name_ == "SimpleBlock" or name_ == "Block":
                    data_=self.format_block(indent_)

                elif data_:
                    encoded_as_text = False
                    if name_ == "CodecPrivate" and self.going_to_be_textual_codec_private:
                        try:
                            data_ = data_.decode("UTF-8")
                            data_ = data_.replace("]]>", "]]]]><![CDATA[>")
                            data_ = "<![CDATA[" + data_ + "]]>"
                            opening_tag_content = "CodecPrivate encoding=\"text\""
                            encoded_as_text = True
                        except UnicodeDecodeError:
                            sys.stderr.write("UTF-8 decoding error, leaving as binary\n")
                    if not encoded_as_text:
                        data_ = maybe_decode(binascii.hexlify(data_))
                        if len(data_) > 40:
                            newdata=""
                            for chunk in chunks(data_, chunklength):
                                newdata+="\n  "+indent_;
                                newdata+=chunk;
                            newdata+="\n"+indent_;
                            data_=newdata

            if type_ == mkvparse.EbmlElementType.MASTER:
                print("%s<%s>"%(indent_,name_))
                self.printtree(data_, indent+1);
                print("%s</%s>"%(indent_, name_))
            elif type_ == mkvparse.EbmlElementType.JUST_GO_ON:
                if name_ == "Segment":
                    if self.there_was_segment:
                        print("</Segment>")
                    print("<Segment>")
                    self.there_was_segment=True
                elif name_ == "Cluster":
                    print("<Cluster>")
                    self.there_was_cluster=True
                    self.indent=1
                else:
                    sys.stderr.write("Unknown JUST_GO_ON element %s\n" % name_)
            else:
                if type_ == mkvparse.EbmlElementType.TEXTA or type_ == mkvparse.EbmlElementType.TEXTU:
                    data_ = saxutils.escape(str(data_))
                print("%s<%s>%s</%s>"%(indent_, opening_tag_content, data_, name_));
            
        

    def ebml_top_element(self, id_, name_, type_, data_):
        self.printtree([(name_, (type_, data_))], self.indent)



# Reads mkv input from stdin, parse it and print details to stdout
banlist=["SeekHead", "CRC-32", "Void", "Cues", "PrevSize", "Position"]
nocluster_mode = False
if len(sys.argv)>1:
    if sys.argv[1]=="-v":
        banlist=[]
    if sys.argv[1]=="-C":
        nocluster_mode = True
        banlist=["SeekHead", "CRC-32", "Void", "Cues", "PrevSize", "Position", "Cluster", "Timecode", "Timestamp", "PrevSize", "EncryptedBlock", "BlockGroup", "SimpleBlock"]

chunklength = int(getenv("CHUNKLENGTH", "64"))

if sys.version >= '3':
    sys.stdin = sys.stdin.detach()

mkvparse.mkvparse(sys.stdin, MatroskaToText(frozenset(banlist), nocluster_mode))

