using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;

public static class BinXml
{
    public static string ToText(byte[] b)
    {
        int p = 0;
        Func<int> u16 = () => { int v = BitConverter.ToUInt16(b, p); p += 2; return v; };
        Func<int> u32 = () => { int v = BitConverter.ToInt32(b, p); p += 4; return v; };
        var names = new Dictionary<int, string>();
        int nn = u16();
        for (int i = 0; i < nn; i++) { int len = u16(); string s = Encoding.UTF8.GetString(b, p, len); p += len; names[u16()] = s; }
        var values = new Dictionary<int, string>();
        int nv = u16();
        for (int i = 0; i < nv; i++) { int len = u16(); string s = Encoding.UTF8.GetString(b, p, len); p += len; values[u16()] = s; }
        var doc = new XmlDocument();
        doc.AppendChild(doc.CreateXmlDeclaration("1.0", "utf-8", null));
        int roots = u16();
        Action<XmlNode> read = null;
        read = parent =>
        {
            int pos0 = p; int nidv = u16();
            if (!names.ContainsKey(nidv)) throw new Exception("bad name " + nidv + " at " + pos0);
            var e = doc.CreateElement(names[nidv]);
            parent.AppendChild(e);
            int has = u32();
            int nc = 0;
            if (has == 1)
            {
                int vid = u32();
                if (!values.ContainsKey(vid)) throw new Exception("bad value " + vid + " at " + pos0);
                e.InnerText = values[vid];
                if (values[vid].Length == 0) e.SetAttribute("_e", "1");
                nc = u16();
                if (nc != 0) throw new Exception("value with children at " + pos0);
            }
            else if (has == 0)
            {
                int x = u32();
                if (x == 1) nc = u16();
                else if (x != 0) throw new Exception("bad x " + x + " at " + pos0);
                else e.SetAttribute("_empty", "1");
            }
            else throw new Exception("bad has " + has + " at " + pos0);
            for (int i = 0; i < nc; i++) read(e);
        };
        for (int i = 0; i < roots; i++) read(doc);
        if (p != b.Length) throw new Exception("not at end: " + p + "/" + b.Length);
        var sb = new StringBuilder();
        var ws = new XmlWriterSettings { Indent = true };
        using (var xw = XmlWriter.Create(sb, ws)) doc.Save(xw);
        return sb.ToString().Replace("encoding=\"utf-16\"", "encoding=\"utf-8\"");
    }

    public static byte[] FromText(string text)
    {
        var doc = new XmlDocument();
        doc.LoadXml(text);
        var names = new List<string>(); var nameIdx = new Dictionary<string, int>();
        var values = new List<string>(); var valIdx = new Dictionary<string, int>();
        var body = new MemoryStream(); var bw = new BinaryWriter(body);
        Func<string, int> nid = s => { int i; if (!nameIdx.TryGetValue(s, out i)) { i = names.Count; names.Add(s); nameIdx[s] = i; } return i; };
        Func<string, int> vidf = s => { int i; if (!valIdx.TryGetValue(s, out i)) { i = values.Count; values.Add(s); valIdx[s] = i; } return i; };
        Action<XmlElement> write = null;
        write = e =>
        {
            var kids = new List<XmlElement>();
            foreach (XmlNode c in e.ChildNodes) if (c is XmlElement) kids.Add((XmlElement)c);
            bw.Write((ushort)nid(e.Name));
            if (kids.Count == 0 && (e.InnerText.Length > 0 || e.HasAttribute("_e"))) { bw.Write(1); bw.Write(vidf(e.InnerText)); bw.Write((ushort)0); }
            else if (kids.Count == 0) { bw.Write(0); bw.Write(0); }
            else { bw.Write(0); bw.Write(1); bw.Write((ushort)kids.Count); foreach (var k in kids) write(k); }
        };
        var rootEls = new List<XmlElement>();
        foreach (XmlNode c in doc.ChildNodes) if (c is XmlElement) rootEls.Add((XmlElement)c);
        bw.Write((ushort)rootEls.Count);
        foreach (var r in rootEls) write(r);
        bw.Flush();
        var outS = new MemoryStream(); var ow = new BinaryWriter(outS);
        ow.Write((ushort)names.Count);
        for (int i = 0; i < names.Count; i++) { var nb = Encoding.UTF8.GetBytes(names[i]); ow.Write((ushort)nb.Length); ow.Write(nb); ow.Write((ushort)i); }
        ow.Write((ushort)values.Count);
        for (int i = 0; i < values.Count; i++) { var vb = Encoding.UTF8.GetBytes(values[i]); ow.Write((ushort)vb.Length); ow.Write(vb); ow.Write((ushort)i); }
        ow.Write(body.ToArray());
        ow.Flush();
        return outS.ToArray();
    }
}
