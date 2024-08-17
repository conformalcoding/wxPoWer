# wxPoWer v0 specification

This document presents a draft specification for wxPoWer version 0.

## Terminology

The following terminology is used in the description of the wxPoWer v0 specification:

- _Body_ (`body`): the message the user wants to submit to the target platform, not including the metadata.
- _Context_ (`context`): the specific context in which the user's message resides, or the message it is a response to (target platform-dependent; see below).
- _Metadata_: the string appended to the body which forms the proof.
- _Proof_: to the user's message body and metadata concatenated together (as per the spec below).
- _Signature_: sometimes used interchangeably with _metadata_.
- _Target platform_: the text-based platform on which the user wants to submit their message (i.e. X, Mastodon, Facebook, YouTube, 4chan, etc.)
- _User identifier_ (`userId`): the user's identifier on the target platform (must be unique).

## Goals

The specification was designed to facilitate the signing of text messages that will be posted on text-based platforms which have limited space. Therefore, the number of fields and the size of field delimiters (and other characters) has been minimized.

## Format

A wxPoWer v0 proof has the form:

```
<body>|<metadata>
```

where `metadata` is specified as:

```
wxPoW0|<userId>|<context>
```

substituting in the variables denoted with angled brackets.

## Field descriptions

### The body

There are no special requirements for this string. No characters are escaped.

### User ID field

The `userId` field is the user's identifier on the target platform (must be unique).

The minimal necessary characters (pipe `|` and backslash `\`) are backslash-escaped in this field.

### Context field

The `context` is likely to be some element of the URL where the message is being posted. For example: on X, if the user intends to submit a response to the post `https://twitter.com/elonmusk/status/1518623997054918657`, then the context should be the smallest element of the URL which makes the context field unique on that particular platform. Therefore, the context should be `1518623997054918657`, as X has universally unique post IDs.

For posts which are not a response but an original post, then the context may be the same as the `userId`, or blank, or equal to the last original post of the user (if this information is easily accessible to all users on the platform).

The minimal necessary characters (pipe `|` and backslash `\`) are backslash-escaped in this field.

## Justification

The design of wxPoWer v0 was influenced by the choice of proof-of-work scheme used.

Unlike using a 'traditional' cryptographic hash function for the purpose, which accepts a single arbitrarily-sized string and produces a fixed-size output hash:

```
out = SHA256(H)
```

where `out` is a fixed-size sequence of 256 bits and `H` is the arbitrarily-sized input string, wxPoWer v0 uses [RandomX (v1.2.1)](https://github.com/tevador/RandomX/releases/tag/v1.2.1):

```
out = RandomX(K, H)
```

It has two inputs: the key `K` (limited in size) and the data to hash `H` (arbitrary size). Both of these inputs influence the resultant 256-bit output hash. The reader is referred to the RX documentation for full details, but a brief overview follows here.

RX's intended usage is that `K` should change relatively infrequently, as it is used to initialize a large dataset subsequently used by the hashing process. (This initialize time is far less than the hashing time for reasonable choices of difficulty.) This helps to give RX the property of ASIC-resistance, and is the primary reason for its use in wxPoWer v0 (as opposed to SHA256, for example, for which specialized hardware can overwhelmingly outperform general-purpose processors).

The RX dataset need not be reinitialized for every `H` attempted. It follows that nonces used to search for a difficult output hash used be concatenated onto `H` only.

To use RX as intended, the metadata string is introduced. This is used as part of the key `K`, as it contains information which is likely to make the string unique. Thus, a more complete expression of RX's usage in pseudocode is:

```
out = RandomX(SHA256(metadata), body + nonce + '|' + metadata)
```

The metadata is hashed using SHA256 to ensure that it fits within the required size bounds of `K`. It must be concatentated to the proof in plaintext so that it the proof can be verified.

Note that wxPoWer v0 uses RandomX v1.2.1 with a modified configuration - see `depends/patch_rx.sh`.

## Difficulty

Difficulty, as defined by this specification, is the number of leading zero bits of the 256-bit sequence `out`. This was decided for simplicity, to keep the difficulty exactly quantifiable as an integer.

The calculation for the probability of a 'win' is trivial under this definition. If the difficulty is defined as $d$, then the probability of a single trial winning is $1/2^d$.

This also allows one to easily calculate the "LD50 hashes" (to crudely borrow the term from medicine). This marks the 50% likelihood of success point. In probabilistic terms: after attempting LD50 hashes, you have had at least half a chance to find a winning hash. If you win after less trials, then you were lucky; if more, then you were unlucky. This can be calculated as $2^{d-1}$ if $d > 0$.

For illustrative purposes, a brief illustration table follows:

Difficulty | Probability of win | LD50 trials
--:|---|--:
0 | $1$ | -
1 | $0.5$ | 1
2 | $0.25$ | 2
3 | $0.125$ | 4
4 | $0.0625$ | 8
5 | $0.03125$ | 16
6 | $0.015625$ | 32
7 | $0.0078125$ | 64
8 | $0.00390625$ | 128
9 | $0.00195312$ | 256
10 | $0.00097656$ | 512
11 | $0.00048828$ | 1024
12 | $0.00024414$ | 2048
13 | $0.00012207$ | 4096
14 | $6.104 \times 10^{-5}$ | 8192
15 | $3.052 \times 10^{-5}$ | 16384
16 | $1.526 \times 10^{-5}$ | 32768
17 | $7.63 \times 10^{-6}$ | 65536
18 | $3.81 \times 10^{-6}$ | 131072
19 | $1.91 \times 10^{-6}$ | 262144
20 | $9.5 \times 10^{-7}$ | 524288

If you also know your hashes per second, then this information can be used to determine how long a proof will likely take to produce.

This is not to be confused with the more flexible but complex approach taken by cryptocurrencies such as Bitcoin. In these, `out` would be considered a 256-bit number and a winning hash occurs when `out` is less than some target value. In practice, difficulties and targets are represented by a truncated floating-point number which cannot represent exact 256-bit values. This is good enough for the purpose, but increases complexity that this spec desires to avoid. There is no need to have such fine-grained control over difficulty.

## Information for implementers

One can search for the last instance of the string `|wxPoW0|` to find the beginning of the wxPoWer v0 signature data.
