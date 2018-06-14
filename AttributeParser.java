import java.util.StringTokenizer;
import java.util.LinkedList;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.ArrayList;

public class AttributeParser
{
    String text;
    int pos;
    Tag currentTag, rootTag;
    private static final String NOT_FOUND = "Not Found!";

    static class Tag
    {
        String name;
        HashMap<String, String> attributes;
        ArrayList<Tag> children;
        Tag parent;

        public Tag(Tag parent, String tagName) {
            name = tagName;
            this.parent = parent;
            attributes = new HashMap<>();
            children = new ArrayList<>();
        }

        public void put(String name, String val) {
            attributes.put(name, val);
        }

        public void addChild(Tag tag) {
            children.add(tag);
        }

        public String get(String attr) {
            return attributes.getOrDefault(attr, NOT_FOUND);
        }

        @Override
        public String toString() {
            var root = this;
            LinkedList<Tag> list = new LinkedList<>();
            while (root != null) {
                list.addFirst(root);
                root = root.parent;
            }
            StringBuilder ans = new StringBuilder();
            for (var t : list)
                ans.append(t.name).append('.');
            return ans.toString();
        }
    }

    private String resolve(String attrName) {

        var index = attrName.indexOf('~');
        var attr = attrName.substring(index+1);
        var tok = new StringTokenizer(attrName.substring(0, index), ".");
        var iterator = rootTag;

        while (tok.hasMoreTokens()) {
            var f = false;
            var dir = tok.nextToken();
            for (var t : iterator.children) {
                if (t.name.equals(dir)) {
                    iterator = t;
                    f = true;
                    break;
                }
            }
            if (!f)
                return NOT_FOUND;
        }

        return iterator.get(attr);
    }

    private final void match(char ch) {
        discard();
        if (text.charAt(pos) == ch) {
            ++pos;
        } else {
            error();
        }
    }

    private final void error() {
        System.out.printf("!! Syntax Error near offset %d !!\n", pos);
        System.out.println(text.substring(pos));
        System.exit(1);
    }

    private String getID() {
        discard();
        var ans = new StringBuilder();
        char ch;
        while (Character.isJavaIdentifierPart(ch = text.charAt(pos))) {
            ++pos;
            ans.append(ch);
        }
        return ans.toString();
    }

    private String getQuotedString() {
        var ans = new StringBuilder();
        discard();
        char ch;
        match('"');
        while ('"' != (ch = text.charAt(pos))) {
            ++pos;
            ans.append(ch);
        }
        match('"');
        return ans.toString();
    }

    private final void discard() {
        var len = text.length();
        while (pos < len && Character.isWhitespace(text.charAt(pos)))
            ++pos;
    }

    private void attribute() {
        var name = getID();
        match('=');
        var val = getQuotedString();
        currentTag.put(name, val);
    }

    private void tagDecl() {
        discard();
        while (pos == text.length() || text.charAt(pos) == '<' && text.charAt(pos+1) == '/')
            return;
        match('<');

        var tagName = getID();
        Tag old = currentTag;
        currentTag = new Tag(old, tagName);

        if (old != null)
            old.addChild(currentTag);

        while (text.charAt(pos) != '>') {
            attribute();
        }
        match('>');

        while (pos+1 < text.length() && text.charAt(pos+1) != '/')
            tagDecl();

        match('<');
        match('/');
        var tagClose = getID();
        match('>');

        currentTag = currentTag.parent;
    }

    private void parse() {
        rootTag = new Tag(null, "/");
        currentTag = rootTag;
        while (pos < text.length())
            tagDecl();
    }

    public static void main(String... argv) throws Throwable {
        var p = new AttributeParser();
        var stdin = new BufferedReader(new InputStreamReader(System.in));

        int lines, q;
        var line = stdin.readLine();
        {
            var pos = line.indexOf(' ');
            lines = Integer.parseInt(line.substring(0, pos));
            q = Integer.parseInt(line.substring(pos+1));
        }

        var b = new StringBuilder();
        while (lines-- > 0) {
            b.append(stdin.readLine());
        }

        p.text = b.toString();
        p.parse();

        while (q-- > 0) {
            System.out.println(p.resolve(stdin.readLine()));
        }
    }
}
