# Pokemon Genetics

This is a feature branch for the pret decompilation of Pokemon Emerald that adds genetics and individual physical variation to Pokemon. These changes are VISUAL ONLY and do not modify the way Pokemon stats are generated or inherited. In practical terms, this means that Pokemon will have a lot more variation in their appearance than we get in vanilla, and that the special elements of a Pokemon's appearance can be inherited by its offspring.

Pokemon can now have different skin/fur colors, different eye colors, alternative patterns, or distinctive physical traits like bigger (or smaller) horns, extra (or less) fur, etc. These new traits can be mixed and matched, giving Pokemon an order of magnitude more individual variation. Because these traits are heritable in a way that mimics real-life genetics (though greatly simplified), it's possible to breed for a particular combination of traits.

While the basic functionality is complete, I have not yet finished creating all the graphics files for each Pokemon species. In the meantime, these files are filled with dummies which will result in the appearance of those Pokemon being as they are in vanilla, regardless of their underlying genetics.

## The basics of Pokemon genetics

I have added a genome to the Pokemon data structure. This takes the form of two 8-bit integers that occupy some of the unused space in the data structure, so this feature should be trade compatible with vanilla games (currently untested). These variables can be thought of as the Pokemon's chromosomes. When Pokemon breed, the baby inherit one half of its genome from the father, and the other half from the mother. Wild Pokemon have their genomes determined randomly. 

Each bit of these variables represents a single gene, and, being a bit, has two possible states - 0 and 1. A 0 represents the wild type variant (allele) for this gene - that is, the normal appearance for that Pokemon species - while a 1 represents an alternative allele. If a Pokemon has two 1's in the same bit of each genome variable, then the alternate trait will manifest in the Pokemon's appearance. Otherwise, it will have the wild type version of that trait. The collection of traits (wild and alternate) which the Pokemon has is referred to as its phenotype. In technical terms, the wild type is dominant, while the alternate traits are recessive.

To make this clearer, let's look at an example. One trait the genome now controls is whether a Pokemon is shiny or not. The wild type for this gene is non-shiny, while the alternate trait is shiny. A Pokemon will only be shiny if the bit in both genome variables that controls the shiny trait is a 1. If either of these bits is a 0, it will be non-shiny. This means that any given Pokemon will be in one of three states with regards to whether it is shiny. 1) The pokemon can have two non-shiny alleles (homozygous dominant), two shiny alleles (homozygous recessive), or one of each (heterozygous).

When two Pokemon breed, a random selection of each parent's genes are passed on to the offspring. There is also a chance that the baby will be born with a mutation - that is, one or more of its genes may be different from the one it would normally have inherited. Because of this, as well as the possibility that parents may be heterozygous, it's possible for a baby Pokemon to be born with a trait that neither parent has.

## So what traits can a Pokemon have?

Pokemon now have genes which control the following traits:
* Shiny - This gene controls whether a Pokemon is shiny or not.
* "Albino" - The Pokemon has an alternate palette. While it is named Albino, the variant palette will not necessarily look like a real-life albino. This palette will often, but not always, be lighter in color than the default palette. There is a shiny and a non-shiny version of this palette.
* "Melanistic" - Just like Albino, but a different palette, usually a darker one. If a Pokemon expresses both the Albino and Melanistic genes (but no Fade genes, see below), Melanistic will override Albino.
* "Albino" Fade - If the Albino gene is also expressed, this gene will cause the Albino palette to be mixed with the normal and/or melanistic palette, depending upon which other genes are being expressed.
* "Melanistic" Fade - This works just like Albino Fade, except with the Melanistic gene and palette.
* Alternate Pattern - If this gene is expressed, some of the colors from whatever palette is produced after the Shiny, Albino, Melanistic, and Fade genes are taken into account will be replaced with a different set of colors. This might result in the Pokemon's eyes changing color, for example. Sometimes, a hidden pattern will appear when this gene is active! The colors replaced do not change if the Pokemon is Shiny.
* Alternate Pattern, Alternate Color - If the Alternate Pattern gene is also expressed, this will change what the colors are changed to. If, for example, the Alternate Pattern gives a Pokemon blue eyes, the Alternate Color gene could make the eyes green instead.
* Special Trait - If a Pokemon has this gene, its sprite is replaced with a different sprite which uses the same palette mappings as the original. This is usually used to give the Pokemon a distinctive physical feature, such as extra spots, a fringe of hair, or longer horns. However, a Special Trait might also give the Pokemon an item, or even change its appearance entirely!

Because of the way these genes can combine, many unique variants of Pokemon can be produced. Where most Pokemon only had two different possible appearances, there can now be dozens! However, because of the way the genes work, some of these changes may be more subtle than others. You may have to look closely to notice the individual differences between two Pokemon!

## I'm confused about the alternate palettes. How do they work, exactly?

For the most part, it's easiest to think of these palettes as being different image layers. The bottom layer is the "base" and all the other layers are laid one on top of the other, replacing or mixing with whatever is visible beneath them. If the gene for a particular layer is being expressed, that layer is visible. Otherwise, it's hidden.

The base layer is the Pokemon's base palette - either normal or shiny, as it would appear in a vanilla game.
Next is the Albino layer. If the Albino Fade gene is active, the Albino layer mixes with the base layer (as if its opacity had been set to 50%).
After that comes the Melanistic layer. If its Fade gene is active, it mixes with the layers beneath. If the Melanistic Fade gene is not active, but the Albino Fade gene is, then the Albino layer gets moved above the Melanistic layer and mixed with it.
Next we have the Alternate Pattern layer. You can think of this layer as being mostly transparent, except where the colors it replaces are. Those get overridden with the color of the Alternate Pattern, determined by whether the Alternate Pattern, Alternate Color gene is active.

## How are genes inherited and expressed?

Though I explained this in broad strokes above, I'd like to go into a bit more detail on how Pokemon pass on their genes to the next generation. The process is fairly simple, but it does require some explanation. Let's say we want to breed a pair of Pokemon with the following genetic composition:

**Father Genotype**
C1: 11111111
C2: 00000000

**Mother Genotype**
C1: 11111111
C2: 11111111

We know that the baby Pokemon will inherit half its genes from its father, and half from its mother, but which half? Does the father contribute one of his chromosomes and the mother contribute one of hers? No. It's more complicated than that. Let's ignore the mother's genes for now. No matter what, she'll be passing on a chromosome that's all 1's. Instead, let's look at happens with the father's genes.

Instead of passing along the entirety of one of his chromosomes, the baby inherits a chromosome that is a random combination of the father's two chromosomes. For each spot in the chromosome, there's a 50% chance that the baby will inherit the gene from the first chromosome, and if not the gene from the first, then it will be the gene from the second chromosome. Combined with the chromosome the baby inherits from its mother, its genome might look like this:

**Baby Genotype**
C1: 10110100 <- From Father
C2: 11111111 <- From Mother

Okay, but what does that mean for how the baby will look? What will its phenotype be? To figure that out, we look at both genes.For each spot in the genome, if either chromosome has a 0, the phenotype will also be a 0. If both are 1's, the phenotype will be a 1. Another way to think of it would be to multiply each column separately. This means that the baby will have the following phenotype:

**Baby Phenotype**
10110100

You might notice that this is exactly the same as the chromosome the father passed down. This is because the mother passed on all 1's. If she had passed on all 0's instead, the baby's phenotype would also be all 0's. In most cases, however, things will not be so simple, and the phenotype will not look like either of its chromosomes.

Now, there is one step of this process that I've overlooked, and that is mutation. For each gene in each chromosome, there is a chance that a 1 will become a 0, and vice-versa. This happens to the chromosomes passed on by the parents, and is determined when the egg is created.

## Current progress:

The coding for this project is completed (unless there are bugs to fix or revisions I decide to make), but I have not yet finished creating all the alternate sprites and palettes. In the meantime, the Pokemon I haven't gotten to yet have sprites and palette files that make them show only their vanilla appearances. To see a list of which Pokemon I've finished modifying, look at "/graphics/pokemon/Sprite Edit Checklist.xls"

One change that I may make in the future is to change the mutation rates for specific genes, at least for wild Pokemon. Currently, wild Pokemon have one mutation rate (which determines their genotype when they're encountered - they mutate a default genome of all 0's), and a different mutation rate for Pokemon produced by breeding. These mutation rates are the same across the entire genome. However, it might be valuable to give each gene index a different mutation rate, allowing the rarity of certain genes to be fine-tuned. For example, currently the shiny gene has the same mutation rate as every other gene. This is great for showing off the variety of palettes I've created, but it does make shinies themselves a lot less special. Forcing a shiny also causes significant lag sometimes, and so it might be better to have wild Pokemon shininess be determined normally, and have the genes change to reflect that.