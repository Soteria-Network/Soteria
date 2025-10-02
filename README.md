<h1 align="center">
Soteria Network [SOTER]  
<br/><br/>
<img src="./src/qt/res/icons/soteria.png" alt="Soteria" width="300"/>
</h1>

# What is Soteria?

Soteria Network based on RVN/BTC Core designed for efficient and interoperable asset management. 
Taking development cues from Ravencoin and latest versions of Bitcoin, Soteria Network currently
employs a completely new variant of dynamic and timestamped energy friendly hashing algorithms, 
SoterG for GPU and SoterC for CPU. The assets in Soteria can be automated using Soteria Smart Plans
allowing the creation of decentralized applications using Lua VM.
The network prioritizes usability, automation, and low fees to make asset minting and management
simple, affordable, and secure. The network's economy runs on SOTER, our native coin that can be
mined on a dual algorithm setup using either GPUs or CPUs.

For detailed information, visit **Website:** [soteria-network.site] https://soteria-network.site

Key Features & Advantages
Ultra-High Performance

    Block reward is 0.18 coin/block with max daily emission equal 1000 coins.
    Lightning-fast 15s block confirmations and buffer cap 13s.
    Industry-leading 1000+ TPS capacity.
    Smart contract supports using Lua VM integrated.
    3 MB blocks for maximum throughput.
    Difficulity adjustment in 2 phases every 5 blocks and every 180 blocks using LMWA-EMA v3.
    Optimized codebase for fast block time and minimum orphan rate.
    Modern C++ features, C++ best practices, best thread and memory management, updated dependencies to almost latest possible versions.
    Efficient and energy friendly algorithms for mining, crafted for Soteria Network.
    Sustainable tokenomics and all fees will be burned quarterly.
    The total supply is so limited that emission is controlled.
    6 months of development until release to bring the best possible features and we will keep developing days and nights.
    
Bank-Grade Security

    FORK ID integrated from genesis to prevent against replay attacks.
    Multi-layered consensus security using best and latest modern practices.
    Advanced cryptographic algorithms.
    Comprehensive testing and audit framework.
    Resistance against common attack vectors, surface attacks, race conditions, warp timestamp attacks and manipulations.

Wallet Features

    IPFS content browser integration.
    Advanced transaction controls and low transaction fees.
    Dusting function integrated to deal with dust amounts.
    Multi-signature support.
    Hardware wallet compatibility.
   

#### Version strategy
Version numbers are following ```major.minor.patch``` semantics.

Soteria is open source and community driven. The development process is publicly visible and anyone can contribute.

### Branches

There are **3** types of branches:

- **master:** *Stable*, contains the code of the latest version of the latest *major.minor* release.
- **maintenance:** Stable, contains the latest version of previous releases,
  which are still under active maintenance. Format: ```<version>-maint```
- **development:** Unstable, contains the latest code under development and the new code for
 upcoming releases. Format: ```<version>-dev```

***Submit your pull requests against `master`***

*Maintenance branches are exclusively mutable by release. When a release is*
*planned, a development branch will be created and commits from master will*
*be cherry-picked into these by maintainers.*

## Contributing 🤝

Please see [the contribution guide](CONTRIBUTING.md) to see how you can
participate in the development of Soteria Core. There are often
[topics seeking help](https://github.com/soteria-network/soteria/labels/help%20wanted)
where your contributions will have high impact and get very appreciation.

If you find a bug or experience issues with this software, please report it
using the [issue system](https://github.com/soteria-network/soteria/issues/new?assignees=&labels=bug&template=bug_report.md&title=%5Bbug%5D+).


### Running on Mainnet

Use this command to start `soteriad` (CLI) on the mainnet.
<code>./soteriad</code>

Use this command to start `soteria-qt` (GUI) on the mainnet.
<code>./soteria-qt</code>




